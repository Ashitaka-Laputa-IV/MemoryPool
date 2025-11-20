/*
 * 教学版内存池实现
 * 
 * 本文件包含MemoryPool类的成员函数实现
 * 使用现代C++特性，注重代码可读性和教学价值
 */

#ifndef MEMORY_POOL_TEACH_TCC
#define MEMORY_POOL_TEACH_TCC

#include <cstdint>
#include <utility>

// ============================================================================
// 构造函数和析构函数实现
// ============================================================================

// 默认构造函数
template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool() noexcept
    : currentBlock(nullptr), currentSlot(nullptr), lastSlot(nullptr), freeSlots(nullptr) {
    // 初始化所有指针为nullptr
}

// 拷贝构造函数 - 不复制内存块，只是创建一个新的空池
template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool& other) noexcept
    : MemoryPool() {
    // 不复制内存块，因为内存池不应该共享内存
    // 每个池应该管理自己的内存
}

// 移动构造函数 - 转移内存块所有权
template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool&& other) noexcept
    : currentBlock(other.currentBlock),
      currentSlot(other.currentSlot),
      lastSlot(other.lastSlot),
      freeSlots(other.freeSlots) {
    // 将源对象的指针设为nullptr，确保析构时不会释放内存
    other.currentBlock = nullptr;
    other.currentSlot = nullptr;
    other.lastSlot = nullptr;
    other.freeSlots = nullptr;
}

// 从不同类型的内存池构造 - 不复制内存，创建新的空池
template <typename T, std::size_t BlockSize>
template <typename U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U>& other) noexcept
    : MemoryPool() {
    // 不同类型的内存池不共享内存，创建新的空池
}

// 析构函数 - 释放所有分配的内存块
template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept {
    // 遍历所有内存块并释放
    slot_pointer curr = currentBlock;
    while (curr != nullptr) {
        slot_pointer prev = curr->next;
        // 使用operator delete释放内存
        ::operator delete(reinterpret_cast<void*>(curr));
        curr = prev;
    }
}

// 移动赋值运算符
template <typename T, std::size_t BlockSize>
MemoryPool<T, BlockSize>& MemoryPool<T, BlockSize>::operator=(MemoryPool&& other) noexcept {
    if (this != &other) {
        // 释放当前对象管理的内存
        this->~MemoryPool();
        
        // 转移所有权
        currentBlock = other.currentBlock;
        currentSlot = other.currentSlot;
        lastSlot = other.lastSlot;
        freeSlots = other.freeSlots;
        
        // 将源对象的指针设为nullptr
        other.currentBlock = nullptr;
        other.currentSlot = nullptr;
        other.lastSlot = nullptr;
        other.freeSlots = nullptr;
    }
    return *this;
}

// ============================================================================
// 辅助函数实现
// ============================================================================

// 计算对齐所需的填充字节数
// 原始版本使用了复杂的位运算，这里使用更直观的实现
template <typename T, std::size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::calculatePadding(data_pointer p, size_type align) const noexcept {
    // 将指针转换为整数地址
    std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(p);
    // 计算需要对齐的字节数
    size_type offset = addr % align;
    // 如果已经对齐，不需要填充；否则需要填充(align - offset)字节
    return (offset == 0) ? 0 : (align - offset);
}

// 分配一个新的内存块
template <typename T, std::size_t BlockSize>
void MemoryPool<T, BlockSize>::allocateNewBlock() {
    // 分配新内存块
    data_pointer newBlock = reinterpret_cast<data_pointer>(::operator new(BlockSize));
    
    // 设置新块的next指针指向当前块
    reinterpret_cast<slot_pointer>(newBlock)->next = currentBlock;
    currentBlock = reinterpret_cast<slot_pointer>(newBlock);
    
    // 计算数据区的起始位置（跳过next指针）
    data_pointer body = newBlock + sizeof(slot_pointer);
    // 计算对齐所需的填充字节数
    size_type bodyPadding = calculatePadding(body, alignof(slot_type));
    
    // 设置当前可用槽的位置
    currentSlot = reinterpret_cast<slot_pointer>(body + bodyPadding);
    // 计算当前块的最后一个槽的位置
    // 块末尾减去一个槽的大小，再加1确保不越界
    lastSlot = reinterpret_cast<slot_pointer>(newBlock + BlockSize - sizeof(slot_type) + 1);
}

// ============================================================================
// 标准分配器接口实现
// ============================================================================

// 获取对象的地址
template <typename T, std::size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference x) const noexcept {
    return &x;
}

// 获取const对象的地址
template <typename T, std::size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference x) const noexcept {
    return &x;
}

// 分配内存
template <typename T, std::size_t BlockSize>
typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type n, const_pointer hint) {
    // 此内存池设计为一次只分配一个对象，忽略n参数
    (void)hint;  // 避免未使用参数警告
    
    // 首先检查是否有空闲槽可以重用
    if (freeSlots != nullptr) {
        // 从空闲槽链表中取出一个槽
        pointer result = reinterpret_cast<pointer>(freeSlots);
        freeSlots = freeSlots->next;
        return result;
    } else {
        // 没有空闲槽，检查当前块是否还有可用槽
        if (currentSlot >= lastSlot) {
            // 当前块已满，分配新块
            allocateNewBlock();
        }
        // 返回当前可用槽，并移动到下一个槽
        return reinterpret_cast<pointer>(currentSlot++);
    }
}

// 释放内存
template <typename T, std::size_t BlockSize>
void MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n) {
    (void)n;  // 忽略n参数，此内存池一次只释放一个对象
    
    if (p != nullptr) {
        // 将释放的槽添加到空闲槽链表的头部
        reinterpret_cast<slot_pointer>(p)->next = freeSlots;
        freeSlots = reinterpret_cast<slot_pointer>(p);
    }
}

// 计算最大可分配的对象数量
// 原始版本使用了"炫技"的-1/BlockSize计算，这里使用更直观的实现
template <typename T, std::size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size() const noexcept {
    // 计算一个块中可容纳的槽的数量
    size_type slotsPerBlock = (BlockSize - sizeof(slot_pointer)) / sizeof(slot_type);
    
    // 返回一个合理的最大值，确保std::vector不会认为分配器容量不足
    // 这里使用一个足够大的值，但不超过size_type的最大值
    return std::numeric_limits<size_type>::max() / sizeof(T);
}

// 获取当前可用的槽数量
template <typename T, std::size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::available() const noexcept {
    size_type count = 0;
    
    // 计算当前块中剩余的槽数量
    if (currentSlot < lastSlot) {
        count += lastSlot - currentSlot;
    }
    
    // 计算空闲槽链表中的槽数量，添加安全限制
    slot_pointer curr = freeSlots;
    while (curr != nullptr && count < 1000) {  // 添加安全限制，防止无限循环
        count++;
        curr = curr->next;
    }
    
    return count;
}

// 获取当前已使用的槽数量
template <typename T, std::size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::used() const noexcept {
    // 计算一个块中可容纳的槽的数量
    size_type slotsPerBlock = (BlockSize - sizeof(slot_pointer)) / sizeof(slot_type);
    
    // 计算总块数
    size_type blockCount = 0;
    slot_pointer curr = currentBlock;
    while (curr != nullptr && blockCount < 1000) {  // 添加安全限制，防止无限循环
        blockCount++;
        curr = curr->next;
    }
    
    // 如果没有块，返回0
    if (blockCount == 0) {
        return 0;
    }
    
    // 总槽数量 = 块数 * 每块槽数量
    size_type totalSlots = blockCount * slotsPerBlock;
    
    // 计算当前块中剩余的槽数量
    size_type availableInCurrentBlock = 0;
    if (currentSlot < lastSlot) {
        availableInCurrentBlock = lastSlot - currentSlot;
    }
    
    // 计算空闲槽链表中的槽数量，添加安全限制
    size_type freeSlotsCount = 0;
    slot_pointer freeSlot = freeSlots;
    while (freeSlot != nullptr && freeSlotsCount < 1000) {  // 添加安全限制
        freeSlotsCount++;
        freeSlot = freeSlot->next;
    }
    
    // 可用槽数量 = 当前块中剩余的槽数量 + 空闲槽链表中的槽数量
    size_type totalAvailable = availableInCurrentBlock + freeSlotsCount;
    
    // 已使用的槽数量 = 总槽数量 - 可用槽数量
    return totalSlots - totalAvailable;
}

// 在指定位置构造对象
template <typename T, std::size_t BlockSize>
template <typename U, typename... Args>
void MemoryPool<T, BlockSize>::construct(U* p, Args&&... args) {
    // 使用placement new在指定位置构造对象
    // 使用完美转发传递参数
    new (p) U(std::forward<Args>(args)...);
}

// 销毁对象
template <typename T, std::size_t BlockSize>
template <typename U>
void MemoryPool<T, BlockSize>::destroy(U* p) {
    // 显式调用析构函数，但不释放内存
    p->~U();
}

// ============================================================================
// 便捷方法实现
// ============================================================================

// 分配并构造一个对象
template <typename T, std::size_t BlockSize>
template <typename... Args>
typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args&&... args) {
    // 分配内存
    pointer result = allocate();
    // 在分配的内存上构造对象
    construct<value_type>(result, std::forward<Args>(args)...);
    return result;
}

// 销毁并释放一个对象
template <typename T, std::size_t BlockSize>
void MemoryPool<T, BlockSize>::deleteElement(pointer p) {
    if (p != nullptr) {
        // 销毁对象
        p->~value_type();
        // 释放内存
        deallocate(p);
    }
}

#endif // MEMORY_POOL_TEACH_TCC