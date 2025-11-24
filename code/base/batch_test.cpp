#include <iostream>
#include <vector>
#include <ctime>
#include "MemoryPool.h"

// 测试用的简单小对象
class SmallObject {
private:
    int data;
    
public:
    SmallObject() : data(0) {}
    SmallObject(int d) : data(d) {}
    ~SmallObject() {}
    
    int getData() const { return data; }
};

// 测试大对象
class TestObject {
private:
    int data[20]; // 80字节的大对象
    
public:
    TestObject() {
        for (int i = 0; i < 20; ++i) {
            data[i] = i;
        }
    }
    
    TestObject(int value) {
        for (int i = 0; i < 20; ++i) {
            data[i] = value + i;
        }
    }
    
    ~TestObject() {}
    
    int getData(int index) const {
        return data[index];
    }
};

// 测试小对象的new/delete性能（批量分配）
clock_t testNewDeleteSmallBatch(int iterations) {
    clock_t start = clock();
    
    // 批量分配
    std::vector<SmallObject*> objects;
    objects.reserve(iterations);
    
    for (int i = 0; i < iterations; ++i) {
        objects.push_back(new SmallObject(i));
    }
    
    // 批量释放
    for (SmallObject* obj : objects) {
        delete obj;
    }
    
    clock_t end = clock();
    return end - start;
}

// 测试小对象的内存池性能（批量分配）
clock_t testMemoryPoolSmallBatch(int iterations) {
    clock_t start = clock();
    
    MemoryPool<SmallObject> pool;
    std::vector<SmallObject*> objects;
    objects.reserve(iterations);
    
    // 批量分配
    for (int i = 0; i < iterations; ++i) {
        objects.push_back(pool.newElement(i));
    }
    
    // 批量释放
    for (SmallObject* obj : objects) {
        pool.deleteElement(obj);
    }
    
    clock_t end = clock();
    return end - start;
}

// 测试大对象的new/delete性能（批量分配）
clock_t testNewDeleteBatch(int iterations) {
    clock_t start = clock();
    
    // 批量分配
    std::vector<TestObject*> objects;
    objects.reserve(iterations);
    
    for (int i = 0; i < iterations; ++i) {
        objects.push_back(new TestObject(i));
    }
    
    // 批量释放
    for (TestObject* obj : objects) {
        delete obj;
    }
    
    clock_t end = clock();
    return end - start;
}

// 测试大对象的内存池性能（批量分配）
clock_t testMemoryPoolBatch(int iterations) {
    clock_t start = clock();
    
    MemoryPool<TestObject> pool;
    std::vector<TestObject*> objects;
    objects.reserve(iterations);
    
    // 批量分配
    for (int i = 0; i < iterations; ++i) {
        objects.push_back(pool.newElement(i));
    }
    
    // 批量释放
    for (TestObject* obj : objects) {
        pool.deleteElement(obj);
    }
    
    clock_t end = clock();
    return end - start;
}

int main() {
    std::cout << "Memory Pool vs new/delete Performance Comparison Test\n";
    std::cout << "=====================================\n\n";
    
    // 测试不同规模的分配/释放操作
    std::vector<int> testSizes;
    testSizes.push_back(10000);
    testSizes.push_back(100000);
    testSizes.push_back(1000000);
    
    for (size_t i = 0; i < testSizes.size(); ++i) {
        int size = testSizes[i];
        std::cout << "Test Scale: " << size << " objects\n";
        std::cout << "-------------------------------------\n";
        
        // 测试大对象
        clock_t newDeleteTime = testNewDeleteBatch(size);
        clock_t memoryPoolTime = testMemoryPoolBatch(size);
        
        std::cout << "Large Object (TestObject - ~80 bytes):\n";
        std::cout << "  new/delete: " << newDeleteTime << " clock ticks\n";
        std::cout << "  memory pool: " << memoryPoolTime << " clock ticks\n";
        
        if (newDeleteTime > 0) {
            double improvement = (double)(newDeleteTime - memoryPoolTime) / newDeleteTime * 100;
            std::cout << "  performance improvement: " << improvement << "%\n";
        }
        
        // 测试小对象
        clock_t newDeleteSmallTime = testNewDeleteSmallBatch(size);
        clock_t memoryPoolSmallTime = testMemoryPoolSmallBatch(size);
        
        std::cout << "\nSmall Object (SmallObject - ~4 bytes):\n";
        std::cout << "  new/delete: " << newDeleteSmallTime << " clock ticks\n";
        std::cout << "  memory pool: " << memoryPoolSmallTime << " clock ticks\n";
        
        if (newDeleteSmallTime > 0) {
            double improvement = (double)(newDeleteSmallTime - memoryPoolSmallTime) / newDeleteSmallTime * 100;
            std::cout << "  performance improvement: " << improvement << "%\n";
        }
        
        std::cout << "\n=====================================\n\n";
    }
    
    std::cout << "Test completed!\n";
    
    return 0;
}