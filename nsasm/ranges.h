#ifndef NSASM_RANGES_H
#define NSASM_RANGES_H

#include <algorithm>
#include <map>
#include <vector>

#include "absl/types/optional.h"
#include "nsasm/address.h"

namespace nsasm {

// A Chunk is a half-open address range (inclusive of first but exclusive of
// second.
using Chunk = std::pair<nsasm::Address, nsasm::Address>;

class DataRange {
 public:
  // Adds bytes to this data range, starting at the given location and running
  // for the requested length.  If this would cross a bank boundary, the write
  // wraps around instead.
  //
  // Returns true if the claimed bytes were previously free, or false if this
  // claim conflicts with a prior claim on this object.
  bool ClaimBytes(nsasm::Address location, int length);

  // As above, but claims a given chunk instead.
  bool ClaimBytes(Chunk chunk);

  // Returns true if the given byte is inside this range.
  bool Contains(nsasm::Address address) const;

  // Get the underlying chunks of this data range.
  const std::vector<Chunk>& Chunks() const { return ranges_; }

 private:
  std::vector<Chunk> ranges_;
};

// Mapping that can track non-overlapping ranges of memory, associating each
// with a value of type T.
template <typename T>
class RangeMap {
 public:
  // If the provided DataRange does not overlap with any prior inserted
  // entries, associate the provided value with all addresses in this range
  // and return true.  Otherwise, return false and do nothing.
  //
  // TODO: Have a means to return a T and address representing the conflict.
  bool Insert(DataRange range, const T& value);

  // Return the T associated with the given address, or nullopt if there is
  // none.
  absl::optional<T> Lookup(nsasm::Address address) const;

  // Returns true if the given byte is inside this range.  Like Lookup, but
  // is faster and doesn't fetch T.
  bool Contains(nsasm::Address address) const {
    return used_.Contains(address);
  }

 private:
  DataRange used_;
  std::map<Chunk, T> mapping_;
};

// Implementation details follow.

template <typename T>
bool RangeMap<T>::Insert(DataRange range, const T& value) {
  DataRange combined = used_;
  for (Chunk chunk : range.Chunks()) {
    if (!combined.ClaimBytes(chunk)) {
      // conflict
      return false;
    }
  }
  // No conflicts, so add this new entry.
  used_ = std::move(combined);
  for (Chunk chunk : range.Chunks()) {
    mapping_.emplace(chunk, value);
  }
  return true;
}

template <typename T>
absl::optional<T> RangeMap<T>::Lookup(nsasm::Address address) const {
  auto it = mapping_.upper_bound(Chunk(address, address));
  if (it != mapping_.end() && it->first.first == address) {
    return it->second;
  }
  if (it == mapping_.begin()) {
    return absl::nullopt;
  }
  --it;
  if (address >= it->first.first && address < it->first.second) {
    return it->second;
  }
  return absl::nullopt;
}

}  // namespace nsasm

#endif  // NSASM_RANGES_H
