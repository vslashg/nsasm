#include "nsasm/ranges.h"

#include <cassert>

namespace nsasm {

namespace {

// Less-than comparator that will compare Chunks with integers,
// using the lower bound of the chunk as the comparison basis.
struct LeftSideSearch {
  bool operator()(const DataRange::Chunk& lhs, int rhs) {
    return lhs.first < rhs;
  }
  bool operator()(int lhs, const DataRange::Chunk& rhs) {
    return lhs < rhs.first;
  }
};

}  // namespace

bool DataRange::ClaimBytes(int location, int length) {
  bool success = true;
  while (length > 0) {
    const int first_byte_next_bank = (location + 0x10000) & 0xff0000;
    const int next_chunk_size =
        std::min(length, first_byte_next_bank - location);
    const Chunk chunk(location, location + next_chunk_size);
    success = DoClaimBytes(chunk) && success;
    length -= next_chunk_size;
    location += next_chunk_size - 0x10000;
  }
  return success;
}

bool DataRange::Contains(int location) const {
  if (ranges_.empty()) {
    return false;
  }
  auto it = std::upper_bound(ranges_.begin(), ranges_.end(), location,
                             LeftSideSearch());
  if (it == ranges_.begin()) {
    return false;
  }
  --it;
  return location >= it->first && location < it->second;
}

bool DataRange::DoClaimBytes(Chunk new_chunk) {
  if (ranges_.empty()) {
    ranges_.push_back(new_chunk);
    return true;
  }

  bool overlaps_existing = false;

  // Find the place in the existing vector of chunks to insert this.
  //
  // This finds the first chunk whose left boundary is strictly greater
  // than new_chunk's left boundary.  This can return a past-the-end iterator.
  auto insertion_point = std::upper_bound(ranges_.begin(), ranges_.end(),
                                          new_chunk.first, LeftSideSearch());

  // If there's a neighbor to the left, see if we can merge with it.
  if (insertion_point != ranges_.begin() &&
      new_chunk.first <= std::prev(insertion_point)->second) {
    --insertion_point;
    if (new_chunk.first < insertion_point->second) {
      overlaps_existing = true;
    }
    insertion_point->second =
        std::max(insertion_point->second, new_chunk.second);
  }
  // Otherwise, see if we can merge to the right.
  else if (insertion_point != ranges_.end() &&
           insertion_point->first <= new_chunk.second) {
    if (insertion_point->first < new_chunk.second) {
      overlaps_existing = true;
    }
    insertion_point->first = new_chunk.first;
    insertion_point->second =
        std::max(insertion_point->second, new_chunk.second);
  }
  // Since this doesn't merge on either side, we can just insert
  // it and be done with it here.
  else {
    ranges_.insert(insertion_point, new_chunk);
    return true;
  }

  // We merged new_chunk into an existing chunk, but we must handle the
  // (unlikely) case that it needs to be merged with further chunks to the
  // right.
  auto merge_end = insertion_point;
  while (++merge_end != ranges_.end()) {
    if (merge_end->first > insertion_point->second) {
      // doesn't merge with the next block
      break;
    }
  }
  if (merge_end > std::next(insertion_point)) {
    if (insertion_point->second > std::next(insertion_point)->first) {
      overlaps_existing = true;
    }
    insertion_point->second =
        std::max(insertion_point->second, std::prev(merge_end)->second);
    ranges_.erase(std::next(insertion_point), merge_end);
  }
  return !overlaps_existing;
}

}  // namespace nsasm