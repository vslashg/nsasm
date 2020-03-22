#ifndef TEST_TEST_SINK_H_
#define TEST_TEST_SINK_H_

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "nsasm/error.h"
#include "nsasm/output_sink.h"

namespace nsasm {

struct ExpectedBytes {
  int location;
  std::vector<std::uint8_t> bytes;
};

class TestSink : public OutputSink {
 public:
  TestSink(std::vector<ExpectedBytes> expected)
      : expected_(std::move(expected)) {}

  ErrorOr<void> Write(nsasm::Address address,
                      absl::Span<const std::uint8_t> data) override;
  ErrorOr<void> Check() const;

 private:
  std::vector<ExpectedBytes> expected_;
  std::map<nsasm::Address, std::uint8_t> received_;
};

}  // namespace nsasm

#endif  // TEST_TEST_SINK_H_
