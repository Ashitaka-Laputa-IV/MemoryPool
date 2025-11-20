/*
 * 教学版内存池实现
 * 
 * 本实现专为C++教学设计，展示如何使用现代C++特性实现一个高效的内存池
 * 相比原始版本，本版本更加注重代码可读性和教学价值
 * 
 * 主要改进：
 * 1. 使用更直观的C++11/14/17特性
 * 2. 添加详细的注释解释内存池工作原理
 * 3. 避免不必要的"炫技"代码
 * 4. 更好的类型安全和错误处理
 */

#ifndef MEMORY_POOL_TEACH_H
#define MEMORY_POOL_TEACH_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

template <typename T, std::size_t BlockSize = 4096>
class MemoryPool {
public:
    // 类型别名，使用C++11的using声明
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    // C++11标准分配器要求的类型
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    
    // 用于重新绑定到不同类型的模板
    template <typename U>
    struct rebind {
        using other = MemoryPool<U>;
    };
    
    // 构造函数和析构函数
    MemoryPool() noexcept;
    MemoryPool(const MemoryPool& other) noexcept;
    MemoryPool(MemoryPool&& other) noexcept;
    
    // 从不同类型的内存池构造
    template <typename U>
    MemoryPool(const MemoryPool<U>& other) noexcept;
    
    ~MemoryPool() noexcept;
    
    // 赋值运算符
    MemoryPool& operator=(const MemoryPool& other) = delete;  // 禁用拷贝赋值
    MemoryPool& operator=(MemoryPool&& other) noexcept;
    
    // 获取对象地址
    pointer address(reference x) const noexcept;
    const_pointer address(const_reference x) const noexcept;
    
    // 分配和释放内存
    // 注意：此内存池一次只能分配一个对象，因此忽略n参数
    pointer allocate(size_type n = 1, const_pointer hint = nullptr);
    void deallocate(pointer p, size_type n = 1);
    
    // 计算最大可分配的对象数量
    size_type max_size() const noexcept;
    
    // 获取当前可用和已使用的槽数量（用于调试和状态检查）
    size_type available() const noexcept;
    size_type used() const noexcept;
    
    // 在指定位置构造对象
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args);
    
    // 销毁对象
    template <typename U>
    void destroy(U* p);
    
    // 便捷方法：分配并构造一个对象
    template <typename... Args>
    pointer newElement(Args&&... args);
    
    // 便捷方法：销毁并释放一个对象
    void deleteElement(pointer p);

private:
    // 内存槽结构，使用union实现内存复用
    union Slot {
        value_type element;  // 存储实际对象
        Slot* next;          // 指向下一个空闲槽
    };
    
    using slot_type = Slot;
    using slot_pointer = Slot*;
    using data_pointer = char*;
    
    // 内存池状态管理
    slot_pointer currentBlock;   // 当前分配的内存块
    slot_pointer currentSlot;     // 当前可用的槽
    slot_pointer lastSlot;        // 当前块的最后一个槽
    slot_pointer freeSlots;       // 空闲槽链表
    
    // 计算对齐所需的填充字节数
    size_type calculatePadding(data_pointer p, size_type align) const noexcept;
    
    // 分配一个新的内存块
    void allocateNewBlock();
    
    // 确保块大小足够容纳至少一个槽
    static_assert(BlockSize >= 2 * sizeof(slot_type), "BlockSize too small.");
};

#include "MemoryPool.tcc"

#endif // MEMORY_POOL_TEACH_H