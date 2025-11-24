#ifndef MEMORY_POOL_BASE
#define MEMORY_POOL_BASE

#include <cstddef>

template <typename T, std::size_t BlockSize = 4096>
class MemoryPool {
public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_rederence = const T&;
  using size_type = std::size_t;
  using difderence_type = std::ptrdiff_t;

  template <typename U>
  struct rebind {
    using other = MemoryPool<U>;
  }

  MemoryPool() noexcept;
  MemoryPool(const MemoryPool& other) noexcept;
  MemoryPool(MemoryPool&& other) noexcept;
  template <typename U>
  MemoryPool(const MemoryPool<U>& other) noexcept;
  ~MemoryPool() noexcept;

  

}
#include "MemoryPool.tcc"

# endif // MEMORY_POOL_BASE