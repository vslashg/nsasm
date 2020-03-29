#ifndef NSASM_EXPRESSION_H_
#define NSASM_EXPRESSION_H_

#include <set>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/types/optional.h"
#include "nsasm/error.h"
#include "nsasm/identifiers.h"
#include "nsasm/numeric_type.h"

namespace nsasm {

struct BinaryOp {
  using FnPtr = ErrorOr<int> (*)(int, int);
  BinaryOp() = default;
  BinaryOp(FnPtr f, char s) : function(f), symbol(s) {}
  explicit operator bool() const { return static_cast<bool>(function); }

  FnPtr function = nullptr;
  char symbol = 0;
};

inline BinaryOp MakePlusOp() {
  BinaryOp::FnPtr op = [](int a, int b) -> ErrorOr<int> { return a + b; };
  return BinaryOp(op, '+');
}
inline BinaryOp MakeMinusOp() {
  BinaryOp::FnPtr op = [](int a, int b) -> ErrorOr<int> { return a - b; };
  return BinaryOp(op, '-');
}
inline BinaryOp MakeMultiplyOp() {
  BinaryOp::FnPtr op = [](int a, int b) -> ErrorOr<int> { return a * b; };
  return BinaryOp(op, '*');
}
inline BinaryOp MakeDivideOp() {
  BinaryOp::FnPtr op = [](int a, int b) -> ErrorOr<int> {
    if (b == 0) {
      return Error("division by zero");
    }
    return a / b;
  };
  return BinaryOp(op, '/');
}

struct UnaryOp {
  using FnPtr = ErrorOr<int> (*)(int);
  using TypeFnPtr = NumericType (*)(NumericType);
  UnaryOp() = default;
  UnaryOp(FnPtr v, TypeFnPtr t, char s)
      : function(v), result_type(t), symbol(s) {}
  explicit operator bool() const { return static_cast<bool>(function); }

  FnPtr function = nullptr;
  TypeFnPtr result_type = nullptr;
  char symbol = 0;
};

inline UnaryOp MakeNegateOp() {
  UnaryOp::FnPtr fn = [](int a) -> ErrorOr<int> { return -a; };
  UnaryOp::TypeFnPtr tfn = [](NumericType t) { return Signed(t); };
  return UnaryOp(fn, tfn, '-');
}
inline UnaryOp MakeLowbyteOp() {
  UnaryOp::FnPtr fn = [](int a) -> ErrorOr<int> { return a & 0xff; };
  UnaryOp::TypeFnPtr tfn = [](NumericType) { return T_byte; };
  return UnaryOp(fn, tfn, '<');
}
inline UnaryOp MakeHighbyteOp() {
  UnaryOp::FnPtr fn = [](int a) -> ErrorOr<int> { return (a >> 8) & 0xff; };
  UnaryOp::TypeFnPtr tfn = [](NumericType) { return T_byte; };
  return UnaryOp(fn, tfn, '>');
}
inline UnaryOp MakeBankbyteOp() {
  UnaryOp::FnPtr fn = [](int a) -> ErrorOr<int> { return (a >> 16) & 0xff; };
  UnaryOp::TypeFnPtr tfn = [](NumericType) { return T_byte; };
  return UnaryOp(fn, tfn, '^');
}

class LookupContext {
 public:
  virtual ErrorOr<int> Lookup(const nsasm::FullIdentifier& id) const = 0;
};

class NullLookupContext : public LookupContext {
 public:
  ErrorOr<int> Lookup(const FullIdentifier&) const override {
    return Error("Can't perform name lookup in this context");
  }
};

class IsLocalContext {
 public:
  // Returns true if the given identifier refers to a local name in this
  // context. An unqualified name that doesn't match a local will evaluate
  // false. A qualified name that is defined in this context will evaluate true.
  virtual bool IsLocal(const nsasm::FullIdentifier& id) const = 0;
};

// Virtual base class representing an argument value.  This can be a constant,
// label, expression, etc.
class Expression {
 public:
  virtual ~Expression() = default;

  // Returns the value of this expression, or an error if it can't be evaluated.
  // (For example, in the case of an unbound label.)
  virtual ErrorOr<int> Evaluate(const LookupContext& context) const = 0;

  // Returns the type of this expression, if known.
  virtual NumericType Type() const = 0;

  // Returns the string value of this expression, iff it is a simple identifier.
  virtual absl::optional<std::string> SimpleIdentifier() const {
    return absl::nullopt;
  }

  // Returns true if this expression requires a name lookup.
  //
  // TODO(): this is not necessary.  We should just Evaluate() with a null
  // context, and see if it succeeds.
  virtual bool RequiresLookup() const = 0;

  // Returns the set of names referenced by this expression that aren't found by
  // the provided local lookup context, and thus must be found in other files.
  virtual std::set<FullIdentifier> ExternalNamesReferenced(
      const IsLocalContext&) const = 0;

  // Returns a human-readable stringized represenation of this argument, coerced
  // to the requested type if provided.
  virtual std::string ToString() const = 0;

 protected:
  // Returns a copy of this expression.
  friend class ExpressionOrNull;
  friend class Label;
  virtual std::unique_ptr<Expression> Copy() const = 0;
};

// Value type that holds an arbitrary Expression, or null.
class ExpressionOrNull : public Expression {
 public:
  ExpressionOrNull() : expr_(nullptr) {}
  template <typename T>
  ExpressionOrNull(std::unique_ptr<T> rhs) : expr_(std::move(rhs)) {}

  ExpressionOrNull(const ExpressionOrNull& rhs)
      : expr_(rhs.expr_ ? rhs.expr_->Copy() : nullptr) {}
  ExpressionOrNull& operator=(const ExpressionOrNull& rhs) {
    expr_ = rhs.expr_ ? rhs.expr_->Copy() : nullptr;
    return *this;
  }
  ExpressionOrNull(ExpressionOrNull&& rhs) = default;
  ExpressionOrNull& operator=(ExpressionOrNull&& rhs) = default;

  explicit ExpressionOrNull(const Expression& rhs) : expr_(rhs.Copy()) {}
  ExpressionOrNull& operator=(const Expression& rhs) {
    expr_ = rhs.Copy();
    return *this;
  }

  explicit operator bool() const { return static_cast<bool>(expr_); }

  ErrorOr<int> Evaluate(const LookupContext& context) const override {
    if (expr_) {
      return expr_->Evaluate(context);
    }
    return Error("logic error: evaluating null expression");
  }

  NumericType Type() const override {
    return expr_ ? expr_->Type() : T_unknown;
  }

  absl::optional<std::string> SimpleIdentifier() const override {
    if (expr_) {
      return expr_->SimpleIdentifier();
    }
    return absl::nullopt;
  }

  virtual bool RequiresLookup() const override {
    return expr_ ? expr_->RequiresLookup() : false;
  }

  virtual std::set<FullIdentifier> ExternalNamesReferenced(
      const IsLocalContext& is_local) const override {
    return expr_ ? expr_->ExternalNamesReferenced(is_local)
                 : std::set<FullIdentifier>();
  }

  std::string ToString() const override {
    return expr_ ? expr_->ToString() : "<NULL>";
  }

  bool IsLabel() const;
  void ApplyLabel(const std::string label);

 private:
  friend class BinaryExpression;
  friend class UnaryExpression;

  std::unique_ptr<Expression> Copy() const override {
    if (!expr_) {
      return absl::make_unique<ExpressionOrNull>();
    } else {
      return absl::make_unique<ExpressionOrNull>(expr_->Copy());
    }
  }

  std::unique_ptr<Expression> expr_;
};

// Literal numeric value.
class Literal : public Expression {
 public:
  explicit Literal(int value, NumericType type = T_unknown)
      : value_(CastTo(type, value)), type_(type) {}

  ErrorOr<int> Evaluate(const LookupContext& context) const override {
    return value_;
  }
  NumericType Type() const override { return type_; }
  virtual bool RequiresLookup() const override { return false; }
  virtual std::set<FullIdentifier> ExternalNamesReferenced(
      const IsLocalContext& is_local) const override {
    return {};
  }
  std::string ToString() const override;

 private:
  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<Literal>(value_, type_);
  }

  int value_;
  NumericType type_;
};

class IdentifierExpression : public Expression {
 public:
  explicit IdentifierExpression(nsasm::FullIdentifier identifier,
                                NumericType type = T_word)
      : type_(type), identifier_(std::move(identifier)) {}

  ErrorOr<int> Evaluate(const LookupContext& context) const override {
    return context.Lookup(identifier_);
  }
  NumericType Type() const override { return type_; }
  virtual bool RequiresLookup() const override { return true; }
  virtual std::set<FullIdentifier> ExternalNamesReferenced(
      const IsLocalContext& is_local) const override {
    if (is_local.IsLocal(identifier_)) {
      return {};
    } else if (identifier_.Qualified()) {
      return {identifier_};
    } else {
      return {FullIdentifier("", identifier_.Identifier())};
    }
  }
  std::string ToString() const override;
  absl::optional<std::string> SimpleIdentifier() const override {
    if (identifier_.Qualified()) {
      return absl::nullopt;
    }
    return identifier_.Identifier();
  }

 private:
  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<IdentifierExpression>(identifier_, type_);
  }

  NumericType type_;
  FullIdentifier identifier_;
};

class BinaryExpression : public Expression {
 public:
  BinaryExpression(ExpressionOrNull lhs, ExpressionOrNull rhs, BinaryOp op)
      : lhs_(std::move(lhs)), rhs_(std::move(rhs)), op_(op) {}

  ErrorOr<int> Evaluate(const LookupContext& context) const override {
    auto lhs_v = lhs_.Evaluate(context);
    auto rhs_v = rhs_.Evaluate(context);
    NSASM_RETURN_IF_ERROR(lhs_v);
    NSASM_RETURN_IF_ERROR(rhs_v);
    return op_.function(*lhs_v, *rhs_v);
  }
  NumericType Type() const override {
    return ArtihmeticConversion(lhs_.Type(), rhs_.Type());
  }
  virtual bool RequiresLookup() const override {
    return lhs_.RequiresLookup() || rhs_.RequiresLookup();
  }
  virtual std::set<FullIdentifier> ExternalNamesReferenced(
      const IsLocalContext& is_local) const override {
    auto result = lhs_.ExternalNamesReferenced(is_local);
    auto rhs_modules = rhs_.ExternalNamesReferenced(is_local);
    result.insert(rhs_modules.begin(), rhs_modules.end());
    return result;
  }
  std::string ToString() const override {
    return absl::StrFormat("op%c(%s, %s)", op_.symbol, lhs_.ToString(),
                           rhs_.ToString());
  }

 private:
  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<BinaryExpression>(lhs_.Copy(), rhs_.Copy(), op_);
  }
  ExpressionOrNull lhs_;
  ExpressionOrNull rhs_;
  BinaryOp op_;
};

class UnaryExpression : public Expression {
 public:
  UnaryExpression(ExpressionOrNull arg, UnaryOp op)
      : arg_(std::move(arg)), op_(op) {}
  ErrorOr<int> Evaluate(const LookupContext& context) const override {
    auto value = arg_.Evaluate(context);
    NSASM_RETURN_IF_ERROR(value);
    return op_.function(*value);
  }
  NumericType Type() const override { return op_.result_type(arg_.Type()); }
  virtual bool RequiresLookup() const override { return arg_.RequiresLookup(); }
  virtual std::set<FullIdentifier> ExternalNamesReferenced(
      const IsLocalContext& is_local) const override {
    return arg_.ExternalNamesReferenced(is_local);
  }
  std::string ToString() const override {
    return absl::StrFormat("op%c(%s)", op_.symbol, arg_.ToString());
  }

 private:
  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<UnaryExpression>(arg_.Copy(), op_);
  }
  ExpressionOrNull arg_;
  UnaryOp op_;
};

// Named label.  Used as a placeholder expression type for disassembly only.
class Label : public Expression {
 public:
  explicit Label(std::string label, std::unique_ptr<Expression>&& expr)
      : label_(std::move(label)), held_value_(std::move(expr)) {}

  ErrorOr<int> Evaluate(const LookupContext& context) const override {
    return held_value_->Evaluate(context);
  }
  NumericType Type() const override { return held_value_->Type(); }
  virtual bool RequiresLookup() const override { return true; }
  virtual std::set<FullIdentifier> ExternalNamesReferenced(
      const IsLocalContext& is_local) const override {
    return {};
  }
  std::string ToString() const override { return label_; }

 private:
  friend class ExpressionOrNull;

  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<Label>(label_, held_value_->Copy());
  }

  std::string label_;
  std::unique_ptr<Expression> held_value_;
};

inline bool ExpressionOrNull::IsLabel() const {
  return dynamic_cast<Label*>(expr_.get());
}

inline void ExpressionOrNull::ApplyLabel(const std::string label) {
  Label* raw_label = dynamic_cast<Label*>(expr_.get());
  if (raw_label) {
    // If we already hold a `Label`, just change it.
    raw_label->label_ = label;
  } else {
    // Construct a Label wrapping our old value
    auto new_expr = absl::make_unique<Label>(label, std::move(expr_));
    expr_ = std::move(new_expr);
  }
}

}  // namespace nsasm

#endif  // NSASM_EXPRESSION_H_