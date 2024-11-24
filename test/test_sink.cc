#include "test/test_sink.h"

namespace nsasm {

ErrorOr<void> TestSink::Write(nsasm::Address address,
                              absl::Span<const std::uint8_t> data) {
  const int size = data.size();
  for (int i = 0; i < size; ++i) {
    nsasm::Address target = address.AddWrapped(i);
    if (received_.count(target)) {
      return Error("Duplicate write to address %s", target.ToString());
    }
    received_[target] = data[i];
  }
  return {};
}

ErrorOr<void> TestSink::Check() const {
  // Local copy of received map; we will clear it as we go
  std::map<nsasm::Address, std::uint8_t> received = received_;

  absl::PrintF("%d %d\n", received.size(), expected_.size());

  for (const ExpectedBytes& entry : expected_) {
    nsasm::Address location(entry.location);
    const std::vector<std::uint8_t>& bytes = entry.bytes;
    const int size = bytes.size();
    for (int i = 0; i < size; ++i) {
      nsasm::Address target = location.AddWrapped(i);
      auto iter = received.find(target);
      if (iter == received.end()) {
        return Error("Expected 0x%02x at %s, but nothing written", bytes[i],
                     target.ToString());
      } else if (iter->second != bytes[i]) {
        return Error("Expected 0x%02x at %s, but 0x%02x was written instead",
                     bytes[i], target.ToString(), iter->second);
      }
      received.erase(iter);
    }
  }
  if (!received.empty()) {
    auto iter = received.begin();
    return Error("Unexpected 0x%02x written at %s", iter->second,
                 iter->first.ToString());
  }
  return {};
}

}  // namespace nsasm
