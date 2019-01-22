#ifndef NSASM_VALUE_H_
#define NSASM_VALUE_H_

#include "nsasm/error.h"
#include "nsasm/numeric_type.h"

namespace nsasm {

// Virtual base class representing an argument value.  This can be a constant,
// label, expression, etc.
class Expression {
 public:
  virtual ~Expression() = default;

  // Returns the value of this expression, or an error if it can't be evaluated.
  // (For example, in the case of an unbound label.)
  virtual ErrorOr<int> Evaluate(Location loc = Location()) const = 0;

  // Returns the type of this expression, if known.
  virtual NumericType Type() const = 0;

  // Returns a human-readable stringized represenation of this argument, coerced
  // to the requested type if provided.
  virtual std::string ToString(NumericType type = N_unknown) const = 0;
};

// Value type that holds an arbitrary Expression, or null.
class ExpressionOrNull : public Expression {
 public:
  ExpressionOrNull() : expr_(nullptr) {}
  template <typename T>
  ExpressionOrNull(std::unique_ptr<T> rhs) : expr_(std::move(rhs)) {}

  ExpressionOrNull(ExpressionOrNull&& rhs) = default;
  ExpressionOrNull& operator=(ExpressionOrNull&& rhs) = default;

  ErrorOr<int> Evaluate(Location loc = Location()) const override {
    if (expr_) {
      return expr_->Evaluate(loc);
    }
    return Error("Logic error: evaluating null expression").SetLocation(loc);
  }

  NumericType Type() const override {
    return expr_ ? expr_->Type() : N_unknown;
  }

  std::string ToString(NumericType type = N_unknown) const override {
    return expr_ ? expr_->ToString(type) : "<NULL>";
  }

  bool IsLabel() const;
  void ApplyLabel(const std::string label);

 private:
  std::unique_ptr<Expression> expr_;
};

// Literal numeric value.
class Literal : public Expression {
 public:
  explicit Literal(int value, NumericType type = N_unknown)
      : value_(CastTo(type, value)), type_(type) {}

  ErrorOr<int> Evaluate(Location loc) const override { return value_; }
  NumericType Type() const override { return type_; }
  std::string ToString(NumericType type) const override;

 private:
  int value_;
  NumericType type_;
};

// Named label.
class Label : public Expression {
 public:
  explicit Label(std::string label, std::unique_ptr<Expression>&& expr)
      : label_(std::move(label)), held_value_(std::move(expr)) {}

  ErrorOr<int> Evaluate(Location loc) const override {
    return held_value_->Evaluate(loc);
  }
  NumericType Type() const override { return held_value_->Type(); }
  std::string ToString(NumericType type) const override { return label_; }

 private:
  friend class ExpressionOrNull;

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