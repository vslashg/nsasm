#ifndef NSASM_ERROR_H_
#define NSASM_ERROR_H_

#include <string>

#include "absl/strings/str_format.h"
#include "absl/types/optional.h"
#include "absl/types/variant.h"
#include "nsasm/location.h"

namespace nsasm {

class ABSL_MUST_USE_RESULT Error {
 public:
  template <typename... Args>
  explicit Error(const absl::FormatSpec<Args...>& format, const Args&... args)
      : message_(absl::StrFormat(format, args...)), location_() {}

  Error& SetLocation(Location location) {
    location_.Update(location);
    return *this;
  }

  // Update from two location objects.  This is intended to accept a path and
  // address, or path and line number.
  Error& SetLocation(Location loc1, Location loc2) {
    location_.Update(loc1);
    location_.Update(loc2);
    return *this;
  }

  std::string ToString() const;

  bool operator==(const Error& rhs) const { return message_ == rhs.message_; }
  bool operator!=(const Error& rhs) const { return message_ != rhs.message_; }

 private:
  std::string message_;
  Location location_;
};

template <typename T>
class ABSL_MUST_USE_RESULT ErrorOr {
 public:
  ErrorOr(T&& t) : value(std::move(t)) {}
  ErrorOr(const T& t) : value(t) {}
  ErrorOr(Error&& e) : value(e) {}
  ErrorOr(const Error& e) : value(e) {}

  const T& operator*() const& { return absl::get<T>(value); }
  T& operator*() & { return absl::get<T>(value); }
  const T&& operator*() const&& { return absl::get<T>(std::move(value)); }
  T&& operator*() && { return absl::get<T>(std::move(value)); }

  const T* operator->() const { return &absl::get<T>(value); }
  T* operator->() { return &absl::get<T>(value); }

  Error error() const { return absl::get<Error>(value); }

  bool ok() const { return absl::holds_alternative<T>(value); }

  bool operator==(const ErrorOr<T>& rhs) const { return value == rhs.value; }
  bool operator!=(const ErrorOr<T>& rhs) const { return value != rhs.value; }

 private:
  absl::variant<Error, T> value;
};

template <>
class ErrorOr<void> {
 public:
  ErrorOr() : value(absl::nullopt) {}
  ErrorOr(Error&& e) : value(e) {}
  ErrorOr(const Error& e) : value(e) {}

  void operator*() const {}

  Error error() const { return *value; }
  bool ok() const { return !value; }

  bool operator==(const ErrorOr<void>& rhs) const { return value == rhs.value; }
  bool operator!=(const ErrorOr<void>& rhs) const { return value != rhs.value; }

 private:
  absl::optional<Error> value;
};

}  // namespace nsasm

#define NSASM_RETURN_IF_ERROR(v) \
  do {                           \
    const auto& eval = v;        \
    if (!eval.ok()) {            \
      return eval.error();       \
    }                            \
  } while (0)

#define NSASM_RETURN_IF_ERROR_WITH_LOCATION(v, ...) \
  do {                                              \
    const auto& eval = v;                           \
    if (!eval.ok()) {                               \
      return eval.error().SetLocation(__VA_ARGS__); \
    }                                               \
  } while (0)

#define NSASM_EXPECT_OK(v, ...)                                  \
  do {                                                           \
    const auto& eval = v;                                        \
    EXPECT_EQ(eval.ok() ? "ok" : eval.error().ToString(), "ok"); \
  } while (0)

#define NSASM_ASSERT_OK(v, ...)                                  \
  do {                                                           \
    const auto& eval = v;                                        \
    ASSERT_EQ(eval.ok() ? "ok" : eval.error().ToString(), "ok"); \
  } while (0)

#endif  // NSASM_ERROR_H_
