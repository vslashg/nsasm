#include "test/test_sink.h"

namespace nsasm {

ErrorOr<void> TestSink::Write(int address,
                              absl::Span<const std::uint8_t> data) {
  for (int i = 0; i < data.size(); ++i) {
    if (received_.count(i + address)) {
      return Error("Duplicate write to address 0x%06x", i);
    }
    received_[i + address] = data[i];
  }
  return {};
}

ErrorOr<void> TestSink::Check() const {
  // Local copy of received map; we will clear it as we go
  std::map<int, std::uint8_t> received = received_;

  absl::PrintF("%d %d\n", received.size(), expected_.size());

  for (const ExpectedBytes& entry : expected_) {
    int location = entry.location;
    const std::vector<std::uint8_t>& bytes = entry.bytes;
    for (int i = 0; i < bytes.size(); ++i) {
      auto iter = received.find(location + i);
      if (iter == received.end()) {
        return Error("Expected 0x%02x at 0x%06x, but nothing written", bytes[i],
                     location + i);
      } else if (iter->second != bytes[i]) {
        return Error(
            "Expected 0x%02x at 0x%06x, but 0x%02x was written instead",
            bytes[i], location + i, iter->second);
      }
      received.erase(iter);
    }
  }
  if (!received.empty()) {
    auto iter = received.begin();
    return Error("Unexpected 0x%02x written at 0x%06x", iter->second,
                 iter->first);
  }
  return {};
}

}  // namespace nsasm
