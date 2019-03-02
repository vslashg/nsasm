#ifndef NSASM_VALUE_H_
#define NSASM_VALUE_H_

#include "nsasm/error.h"
#include "nsasm/numeric_type.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/types/optional.h"

namespace nsasm {

struct BinaryOp {
  ErrorOr<int> (*function)(int, int) = nullptr;
  char symbol;
  explicit operator bool() const { return static_cast<bool>(function); }
};
ABSL_CONST_INIT inline BinaryOp plus_op{
    [](int a, int b) -> ErrorOr<int> { return a + b; }, '+'};
ABSL_CONST_INIT inline BinaryOp minus_op{
    [](int a, int b) -> ErrorOr<int> { return a - b; }, '-'};
ABSL_CONST_INIT inline BinaryOp multiply_op{
    [](int a, int b) -> ErrorOr<int> { return a * b; }, '*'};
ABSL_CONST_INIT inline BinaryOp divide_op{[](int a, int b) -> ErrorOr<int> {
                                            if (b == 0) {
                                              return Error("division by zero");
                                            }
                                            return a / b;
                                          },
                                          '/'};

struct UnaryOp {
  ErrorOr<int> (*function)(int) = nullptr;
  char symbol;
  explicit operator bool() const { return static_cast<bool>(function); }
};
ABSL_CONST_INIT inline UnaryOp negate_op{
    [](int a) -> ErrorOr<int> { return -a; }, '-'};

// Virtual base class representing an argument value.  This can be a constant,
// label, expression, etc.
class Expression {
 public:
  virtual ~Expression() = default;

  // Returns the value of this expression, or an error if it can't be evaluated.
  // (For example, in the case of an unbound label.)
  virtual ErrorOr<int> Evaluate() const = 0;

  // Returns the type of this expression, if known.
  virtual NumericType Type() const = 0;

  // Returns the string value of this expression, iff it is a simple identifier.
  virtual absl::optional<std::string> SimpleIdentifier() const {
    return absl::nullopt;
  }

  // Returns true if this expression requires a name lookup.
  virtual bool RequiresLookup() const = 0;

  // Returns a human-readable stringized represenation of this argument, coerced
  // to the requested type if provided.
  virtual std::string ToString(NumericType type = T_unknown) const = 0;

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

  ErrorOr<int> Evaluate() const override {
    if (expr_) {
      return expr_->Evaluate();
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

  std::string ToString(NumericType type = T_unknown) const override {
    return expr_ ? expr_->ToString(type) : "<NULL>";
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

  ErrorOr<int> Evaluate() const override { return value_; }
  NumericType Type() const override { return type_; }
  virtual bool RequiresLookup() const override { return false; }
  std::string ToString(NumericType type) const override;

 private:
  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<Literal>(value_, type_);
  }

  int value_;
  NumericType type_;
};

// Unresolved identifier
class Identifier : public Expression {
 public:
  explicit Identifier(std::string identifier)
      : identifier_(std::move(identifier)) {}

  ErrorOr<int> Evaluate() const override {
    return Error("can't resolve identifier %s", identifier_);
  }
  NumericType Type() const override { return T_unknown; }
  virtual bool RequiresLookup() const override { return true; }
  std::string ToString(NumericType type) const override { return identifier_; }
  absl::optional<std::string> SimpleIdentifier() const override {
    return identifier_;
  }

 private:
  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<Identifier>(identifier_);
  }

  std::string identifier_;
};

class BinaryExpression : public Expression {
 public:
  BinaryExpression(ExpressionOrNull lhs, ExpressionOrNull rhs, BinaryOp op)
      : lhs_(std::move(lhs)), rhs_(std::move(rhs)), op_(op) {}

  ErrorOr<int> Evaluate() const override {
    auto lhs_v = lhs_.Evaluate();
    auto rhs_v = rhs_.Evaluate();
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
  std::string ToString(NumericType type) const override {
    return absl::StrFormat("op%c(%s, %s)", op_.symbol, lhs_.ToString(type),
                           rhs_.ToString(type));
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
  ErrorOr<int> Evaluate() const override {
    auto value = arg_.Evaluate();
    NSASM_RETURN_IF_ERROR(value);
    return op_.function(*value);
  }
  NumericType Type() const override { return Signed(arg_.Type()); }
  virtual bool RequiresLookup() const override { return arg_.RequiresLookup(); }
  std::string ToString(NumericType type) const override {
    return absl::StrFormat("op%c(%s)", op_.symbol, arg_.ToString(type));
  }

 private:
  std::unique_ptr<Expression> Copy() const override {
    return absl::make_unique<UnaryExpression>(arg_.Copy(), op_);
  }
  ExpressionOrNull arg_;
  UnaryOp op_;
};

// Named label.
class Label : public Expression {
 public:
  explicit Label(std::string label, std::unique_ptr<Expression>&& expr)
      : label_(std::move(label)), held_value_(std::move(expr)) {}

  ErrorOr<int> Evaluate() const override {
    return held_value_->Evaluate();
  }
  NumericType Type() const override { return held_value_->Type(); }
  virtual bool RequiresLookup() const override { return true; }
  std::string ToString(NumericType type) const override { return label_; }

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

#endif  // NSASM_VALUE_H_