# ifndef _H_CINNAMON_ALLOCATOR_
# define _H_CINNAMON_ALLOCATOR_

#include<vector>
#include <stdexcept>


namespace SST {
namespace Cinnamon {
namespace Utils {

class PoolAllocator {
 public:
  PoolAllocator(size_t numObjects)
    : numObjects(numObjects) {
        table.resize(numObjects);
        for( size_t i = 0; i < numObjects; i++) {
            table[i] = i+1;
        }
    }

  uint32_t allocate() {
    auto freeIndex = mAlloc;
    mAlloc = table[mAlloc];
    if(freeIndex >= numObjects ){
        throw std::runtime_error("Allocation Failed. No free space available");
    }
    return freeIndex;
  }

  void deallocate(uint32_t index) {
        table[index] = mAlloc;
        mAlloc = index;
  }
 
 private:

  size_t numObjects = 0;
  std::vector<uint32_t> table;
  uint32_t mAlloc;
};

} //namespace Utils
} //namespace Cinnamon
} //namespace SST


# endif //_H_CINNAMON_ALLOCATOR_