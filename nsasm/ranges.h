#ifndef NSASM_RANGES_H
#define NSASM_RANGES_H

#include <algorithm>
#include <vector>

namespace nsasm {

class DataRange {
 public:
  using Chunk = std::pair<int, int>;

  // Adds bytes to this data range, starting at the given location and running
  // for the requested length.  If this would cross a bank boundary, the write
  // wraps around instead.
  //
  // Returns true if the claimed bytes were previously free, or false if this
  // claim conflicts with a prior claim on this object.
  bool ClaimBytes(int location, int length);

  // Returns true if the given byte is inside this range.
  bool Contains(int address) const;

  // Get the underlying chunks of this data range.
  const std::vector<Chunk>& Chunks() const { return ranges_; }

 private:
  bool DoClaimBytes(Chunk new_chunk);
  std::vector<Chunk> ranges_;
};

}  // namespace nsasm

#endif  // NSASM_RANGES_H
