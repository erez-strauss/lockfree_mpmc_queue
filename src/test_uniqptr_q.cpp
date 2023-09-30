// Test, example using unique_ptr<> as the value transferred in the lock free queue.
// Compile using: g++ -std=c++20 -mcx16 -I. -Wall -pedantic -ggdb3 -o pointer_test pointer_test.cpp 
#include <pointer_mpmc_queue.h>

#include <cassert>

class Data
{
public:
    Data(int64_t v = 0) : _data(v) {}
    ~Data() { std::cout << "end of data: " << _data << " @" << (void*)this << std::endl; }
    int64_t _data;
};

void test1()
{
    static_assert(8 == sizeof(std::unique_ptr<Data>), "sizeof(std::unique_ptr<T>) must be 8 bytes");
    std::cout << "sizeof std::unique_ptr<Data>: " << sizeof(std::unique_ptr<Data>) << std::endl;
    es::lockfree::pointer_mpmc_queue<es::lockfree::mpmc_queue, Data> tmq(64);
    for (int i = 0; i < 5; ++i)
    {
        tmq.push(std::make_unique<Data>(i * i));
    }
    for (int i = 0; i < 4; ++i)
    {
        std::unique_ptr<Data> p{};
        tmq.pop(p);
        if (!p)
            std::cout << "Error: empty unique pointer: " << i << std::endl;
        else if (p->_data != i * i)
            std::cout << "Error: poped value incorrecti: " << i << " value: " << p->_data << std::endl;
        else
            std::cout << "poped value ok: " << i << std::endl;
    }
    es::lockfree::pointer_mpmc_queue<es::lockfree::mpmc_queue, Data> q1(4);
    es::lockfree::pointer_mpmc_queue<es::lockfree::mpmc_queue, Data> q2(4096);
}

void test2()
{
    struct NoneMovable
    {
        NoneMovable(NoneMovable&&) = delete;
    };
    es::lockfree::pointer_mpmc_queue<es::lockfree::mpmc_queue, NoneMovable> qut(64);
    assert(qut.size() == 0);
    assert(qut.empty() == true);
    assert(qut.capacity() == 64);
}

void test3()
{
    struct NoneMovable
    {
        NoneMovable(long x = 0) : _data(x){};
        NoneMovable(NoneMovable&) = delete;
        long _data{0};
    };
    es::lockfree::pointer_mpmc_queue<es::lockfree::mpmc_queue, NoneMovable> qut(4);
    assert(qut.size() == 0);
    assert(qut.empty() == true);
    assert(qut.capacity() == 4);
    qut.push(std::make_unique<NoneMovable>());
    assert(qut.size() == 1);
    assert(qut.empty() == false);
    assert(qut.capacity() == 4);
    qut.push(std::make_unique<NoneMovable>());
    qut.push(std::make_unique<NoneMovable>());
    qut.push(std::make_unique<NoneMovable>());
    qut.push(std::make_unique<NoneMovable>());  // will fail
    qut.push(std::make_unique<NoneMovable>());  //
    assert(qut.size() == 4);
    assert(qut.empty() == false);
    assert(qut.capacity() == 4);
}

int main()
{
    test1();
    test2();
    test3();
    return 0;
}
