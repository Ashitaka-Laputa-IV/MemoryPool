#ifndef MEMORY_POOL_BASE_H
#define MEMORY_POOL_BASE_H

#include <cstddef>

template <typename T, std::size_t BlockSize = 4096> // default 4096
class MemoryPool {
public:
  // type alias
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // rebind to a different template type
  template <typename U>
  struct rebind {
    using other = MemoryPool<U>;
  };
  
  // constructor and destructor
  MemoryPool() noexcept;
  MemoryPool(const MemoryPool& other) = delete;
  MemoryPool(MemoryPool&& other) noexcept;
  template <typename U>
  MemoryPool(const MemoryPool<U>& other) = delete;  // construct it but from different type
  ~MemoryPool() noexcept;

  // opeartor=
  MemoryPool& operator=(const MemoryPool& other) = delete;  //deny
  MemoryPool& operator=(MemoryPool&& other) noexcept;

  // get the address
  pointer address(reference x) const noexcept;
  const_pointer address(const_reference x) const noexcept;

  // allocate and deallocate
  pointer allocate();  // only allows single-object allocation  
  void deallocate(pointer p);

  // construct/destroy an object in-place at the specified location
  template<typename U, typename... Args>
  void construct(U* p, Args&&... args);
  template<typename U>
  void destroy(U* p);

  // convenience method: allocates and constructs an object, destroys and frees an object
  template<typename... Args>
  pointer newElement(Args&&... args);
  void deleteElement(pointer p);

private:
  // memory slot
  union Slot {
    value_type element;
    Slot* next;
  };

  // type alias
  using slot_type = Slot;
  using slot_pointer = Slot*;
  using data_pointer = char*;

  // variable for managing memory pool
  slot_pointer currentBlock;   // currently allocated memory block
  slot_pointer currentSlot;     // currently available slot
  slot_pointer lastSlot;        // last slot in the current block
  slot_pointer freeSlots;       // free slot list

  // calculates the number of alignment padding bytes
  size_type calculatePadding(data_pointer p, size_t align) const noexcept;

  // allocates a new memory block
  void allocateNewBlock();

  // verifies block can hold at least one slot
  static_assert(BlockSize >= 2 * sizeof(slot_type), "BlockSize too SMALL!");
};

#include "MemoryPool.tcc"

# endif // MEMORY_POOL_BASE_H
