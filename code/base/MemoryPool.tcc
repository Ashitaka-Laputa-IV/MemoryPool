#ifndef MEMORY_POOL_BASE_TCC
#define MEMORY_POOL_BASE_TCC

#include <cstddef>
#include <cstdint>
#include <new>

template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool() noexcept 
  : currentBlock(nullptr)
  , currentSlot(nullptr)
  , lastSlot(nullptr)
  , freeSlots(nullptr) {
  // initialize all pointer with nullptr
}

template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool&& other) noexcept 
  : currentBlock(other.currentBlock)
  , currentSlot(other.currentSlot)
  , lastSlot(other.lastSlot)
  , freeSlots(other.freeSlots) {
  other.currentBlock = nullptr;
  other.currentSlot = nullptr;
  other.lastSlot = nullptr;
  other.freeSlots = nullptr;
}

template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept {
  slot_pointer curr = currentBlock;
  while (curr != nullptr) {
    slot_pointer successor = curr->next;
    ::operator delete(reinterpret_cast<void*>(curr));
    curr = successor;
  }
  currentBlock = nullptr;
  currentSlot = nullptr;
  lastSlot = nullptr;
  freeSlots = nullptr;
}

template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>& MemoryPool<T, BlockSize>::operator=(MemoryPool&& other) noexcept {
  if (this != &other) {
    this->~MemoryPool();

    currentBlock = other.currentBlock;
    currentSlot = other.currentSlot;
    lastSlot = other.lastSlot;
    freeSlots = other.freeSlots;

    other.currentBlock = nullptr;
    other.currentSlot = nullptr;
    other.lastSlot = nullptr;
    other.freeSlots = nullptr;
  }
  return *this;
}

template <typename T, std::size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type MemoryPool<T, BlockSize>::calculatePadding(data_pointer p, size_t align) const noexcept {
  std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(p);
  size_type offset = addr % align;
  return (offset == 0) ? 0 : (align - offset);
}

template <typename T, std::size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer MemoryPool<T, BlockSize>::address(reference x) const noexcept {
  return &x;
}

template <typename T, std::size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer MemoryPool<T, BlockSize>::address(const_reference x) const noexcept {
  return &x;
}

template <typename T, std::size_t BlockSize>
typename MemoryPool<T, BlockSize>::pointer MemoryPool<T, BlockSize>::allocate() {
  // allocation priority: freeSlots -> currentSlot -> new block
  if (freeSlots != nullptr) { 
    pointer pos = reinterpret_cast<pointer>(freeSlots);
    freeSlots = freeSlots->next;
    return pos;
  } else if (currentSlot >= lastSlot) {
    allocateNewBlock();
  }
  return reinterpret_cast<pointer>(currentSlot++);
}

template <typename T, std::size_t BlockSize>
void MemoryPool<T, BlockSize>::deallocate(pointer p) {
  // add to head
  if (p != nullptr) {
    reinterpret_cast<slot_pointer>(p)->next = freeSlots;
    freeSlots = reinterpret_cast<slot_pointer>(p);
  }
}

template <typename T, std::size_t BlockSize>
template<typename U, typename... Args>
void MemoryPool<T, BlockSize>::construct(U* p, Args&&... args) {
  new (p) U(std::forward<Args>(args)...);
}

template <typename T, std::size_t BlockSize>
template<typename U>
void MemoryPool<T, BlockSize>::destroy(U* p) {
  p->~U();
}

template <typename T, std::size_t BlockSize>
template<typename... Args>
typename MemoryPool<T, BlockSize>::pointer MemoryPool<T, BlockSize>::newElement(Args&&... args) {
  pointer p = allocate();
  construct<value_type>(p, std::forward<Args>(args)...);
  return p;
}

template <typename T, std::size_t BlockSize>
void MemoryPool<T, BlockSize>::deleteElement(pointer p) {
  if (p != nullptr) {
    p->~value_type();
    deallocate(p);
  }
}

template <typename T, std::size_t BlockSize>
void MemoryPool<T, BlockSize>::allocateNewBlock() {
  data_pointer newBlock = reinterpret_cast<data_pointer>(::operator new(BlockSize));

  reinterpret_cast<slot_pointer>(newBlock)->next = currentBlock;
  currentBlock = reinterpret_cast<slot_pointer>(newBlock);

  data_pointer body = newBlock + sizeof(slot_pointer);
  size_type bodyPadding = calculatePadding(body, alignof(slot_type));
  currentSlot = reinterpret_cast<slot_pointer>(body + bodyPadding);
  lastSlot = reinterpret_cast<slot_pointer>(newBlock + BlockSize - sizeof(slot_type) + 1);
}

#endif // MEMORY_POOL_BASE_TCC
