/*
 * Memory Pool Performance Test
 * 
 * This file is used to test the performance of MemoryPool and compare it with the standard allocator
 * Test contents include:
 * 1. Time consumption for allocating and deallocating a large number of small objects
 * 2. Time consumption for random allocation and deallocation
 * 3. Performance comparison in case of memory fragmentation
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <functional>
#include "MemoryPool.h"

// Test object class
class TestObject {
public:
    int value;
    double data[8]; // Increase object size
    
    TestObject(int v = 0) : value(v) {
        for (int i = 0; i < 8; ++i) {
            data[i] = static_cast<double>(v + i);
        }
    }
    
    void doSomething() {
        value++;
        for (int i = 0; i < 8; ++i) {
            data[i] *= 1.001;
        }
    }
};

// Test function type
using TestFunction = std::function<void()>;

// Execute test and measure time
double measureTime(TestFunction func, const std::string& testName) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << std::setw(30) << std::left << testName << ": " 
              << duration.count() << " microseconds" << std::endl;
    
    return static_cast<double>(duration.count());
}

// Test 1: Large number of sequential allocations and deallocations
void testSequentialAllocation() {
    const int numObjects = 100000;
    
    // Using standard allocator
    std::cout << "\n=== Test 1: Sequential allocation and deallocation of " << numObjects << " objects ===" << std::endl;
    
    double standardTime = measureTime([&]() {
        std::vector<TestObject*> objects;
        objects.reserve(numObjects);
        
        // Allocate
        for (int i = 0; i < numObjects; ++i) {
            objects.push_back(new TestObject(i));
        }
        
        // Use objects
        for (auto obj : objects) {
            obj->doSomething();
        }
        
        // Deallocate
        for (auto obj : objects) {
            delete obj;
        }
    }, "Standard allocator");
    
    // Using memory pool
    double poolTime = measureTime([&]() {
        MemoryPool<TestObject> pool;
        std::vector<TestObject*> objects;
        objects.reserve(numObjects);
        
        // Allocate
        for (int i = 0; i < numObjects; ++i) {
            objects.push_back(pool.newElement(i));
        }
        
        // Use objects
        for (auto obj : objects) {
            obj->doSomething();
        }
        
        // Deallocate
        for (auto obj : objects) {
            pool.deleteElement(obj);
        }
    }, "Memory pool allocator");
    
    // Calculate performance improvement
    double improvement = (standardTime - poolTime) / standardTime * 100.0;
    std::cout << std::setw(30) << std::left << "Performance improvement" << ": " 
              << std::fixed << std::setprecision(2) << improvement << "%" << std::endl;
}

// Test 2: Random allocation and deallocation
void testRandomAllocation() {
    const int numOperations = 50000;
    
    std::cout << "\n=== Test 2: Random allocation and deallocation with " << numOperations << " operations ===" << std::endl;
    
    // Prepare random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    // Using standard allocator
    double standardTime = measureTime([&]() {
        std::vector<TestObject*> objects;
        
        for (int i = 0; i < numOperations; ++i) {
            if (dis(gen) % 3 == 0 && !objects.empty()) {
                // Randomly deallocate an object
                int idx = dis(gen) % objects.size();
                delete objects[idx];
                objects.erase(objects.begin() + idx);
            } else {
                // Allocate new object
                objects.push_back(new TestObject(i));
            }
        }
        
        // Clean up remaining objects
        for (auto obj : objects) {
            delete obj;
        }
    }, "Standard allocator");
    
    // Using memory pool
    double poolTime = measureTime([&]() {
        MemoryPool<TestObject> pool;
        std::vector<TestObject*> objects;
        
        for (int i = 0; i < numOperations; ++i) {
            if (dis(gen) % 3 == 0 && !objects.empty()) {
                // Randomly deallocate an object
                int idx = dis(gen) % objects.size();
                pool.deleteElement(objects[idx]);
                objects.erase(objects.begin() + idx);
            } else {
                // Allocate new object
                objects.push_back(pool.newElement(i));
            }
        }
        
        // Clean up remaining objects
        for (auto obj : objects) {
            pool.deleteElement(obj);
        }
    }, "Memory pool allocator");
    
    // Calculate performance improvement
    double improvement = (standardTime - poolTime) / standardTime * 100.0;
    std::cout << std::setw(30) << std::left << "Performance improvement" << ": " 
              << std::fixed << std::setprecision(2) << improvement << "%" << std::endl;
}

// Test 3: Using STL containers
void testWithSTLContainer() {
    const int numElements = 50000;
    
    std::cout << "\n=== Test 3: Using with STL containers (" << numElements << " elements) ===" << std::endl;
    
    // Using standard allocator
    double standardTime = measureTime([&]() {
        std::vector<TestObject> vec;
        vec.reserve(numElements);
        
        for (int i = 0; i < numElements; ++i) {
            vec.emplace_back(i);
        }
        
        for (auto& obj : vec) {
            obj.doSomething();
        }
    }, "std::vector (default allocator)");
    
    // Using memory pool allocator
    double poolTime = measureTime([&]() {
        std::vector<TestObject, MemoryPool<TestObject>> vec;
        vec.reserve(numElements);
        
        for (int i = 0; i < numElements; ++i) {
            vec.emplace_back(i);
        }
        
        for (auto& obj : vec) {
            obj.doSomething();
        }
    }, "std::vector (memory pool allocator)");
    
    // Calculate performance improvement
    double improvement = (standardTime - poolTime) / standardTime * 100.0;
    std::cout << std::setw(30) << std::left << "Performance improvement" << ": " 
              << std::fixed << std::setprecision(2) << improvement << "%" << std::endl;
}

// Test 4: Memory pool status monitoring
void testMemoryPoolStatus() {
    std::cout << "\n=== Test 4: Memory pool status monitoring ===" << std::endl;
    
    MemoryPool<TestObject> pool;
    
    std::cout << "Initial state:" << std::endl;
    std::cout << "  Available slots: " << pool.available() << std::endl;
    std::cout << "  Used slots: " << pool.used() << std::endl;
    
    // Allocate some objects
    std::vector<TestObject*> objects;
    for (int i = 0; i < 100; ++i) {
        objects.push_back(pool.newElement(i));
    }
    
    std::cout << "\nAfter allocating 100 objects:" << std::endl;
    std::cout << "  Available slots: " << pool.available() << std::endl;
    std::cout << "  Used slots: " << pool.used() << std::endl;
    
    // Deallocate half of the objects
    for (int i = 0; i < 50; ++i) {
        pool.deleteElement(objects[i]);
    }
    
    std::cout << "\nAfter deallocating 50 objects:" << std::endl;
    std::cout << "  Available slots: " << pool.available() << std::endl;
    std::cout << "  Used slots: " << pool.used() << std::endl;
    
    // Clean up remaining objects
    for (int i = 50; i < 100; ++i) {
        pool.deleteElement(objects[i]);
    }
    
    std::cout << "\nAfter deallocating all objects:" << std::endl;
    std::cout << "  Available slots: " << pool.available() << std::endl;
    std::cout << "  Used slots: " << pool.used() << std::endl;
}

int main() {
    std::cout << "Memory Pool Performance Test" << std::endl;
    std::cout << "==========================" << std::endl;
    
    // Run all tests
    testSequentialAllocation();
    testRandomAllocation();
    testWithSTLContainer();
    testMemoryPoolStatus();
    
    std::cout << "\nAll tests completed!" << std::endl;
    
    return 0;
}