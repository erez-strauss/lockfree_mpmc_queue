//
// Lockfree, atomic, multi producer, multi consumer queue
//  mpmc queue functionality tests utility
//
// MIT License
//
// Copyright (c) 2019 Erez Strauss, erez@erezstrauss.com
//  http://github.com/erez-strauss/lockfree_mpmc_queue/
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <hash_signature.h>
#include <mpmc_queue.h>
//
#include <gtest/gtest.h>
//
#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

struct Globals
{
    bool verbose{false};
};

Globals gl{};

#define VERBOSE(X)      \
    do                  \
    {                   \
        if (gl.verbose) \
        {               \
            X;          \
        }               \
    } while (0)

template<typename QT>
inline void testPushPop(QT &qut)
{
    // Queue under test

    typename QT::value_type w{199};
    typename QT::value_type r{0};
    VERBOSE(std::cout << __PRETTY_FUNCTION__ << '\n');
    qut.push(w);
    qut.pop(r);
    EXPECT_EQ(w, r);
    typename QT::value_type wq{199};
    typename QT::value_type rq{0};
    qut.enqueue(wq);
    qut.dequeue(rq);
    EXPECT_EQ(wq, rq);
}

TEST(MPMC_Queue_FunctionalityTest, PushOne)
{
    es::lockfree::mpmc_queue<int, 0, unsigned, false, false> q0(8);
    es::lockfree::mpmc_queue<int, 0, unsigned, false, true>  q1(8);
    es::lockfree::mpmc_queue<int, 0, unsigned, true, false>  q2(8);
    es::lockfree::mpmc_queue<int, 0, unsigned, true, true>   q3(8);

    testPushPop(q0);
    testPushPop(q1);
    testPushPop(q2);
    testPushPop(q3);

    q0.push(3);
    q1.push(4);
    q2.push(5);
    q3.push(6);

    int a{0}, b{0}, c{0}, d{0};
    q0.pop(a);
    q1.pop(b);
    q2.pop(c);
    q3.pop(d);

    EXPECT_TRUE(a == 3 && b == 4 && c == 5 && d == 6);
}

template<typename QT>
inline void testFullEmpty1(QT &qut)
{
    typename QT::value_type w{0};
    typename QT::value_type r{0};
    unsigned                wloop{0};
    unsigned                rloop{0};

    VERBOSE(std::cerr << __PRETTY_FUNCTION__ << " q capacity: " << qut.capacity() << '\n');
    for (wloop = 0; qut.push(w); wloop++) w++;
    for (rloop = 0; qut.pop(r) && r == static_cast<typename QT::value_type>(rloop); rloop++)
        ;
    for (wloop = 0; qut.push(w); wloop++) w++;
    for (rloop = 0; qut.pop(r) && r == static_cast<typename QT::value_type>(rloop + qut.capacity()); rloop++)
        ;

    EXPECT_EQ(wloop, rloop);
    EXPECT_EQ(wloop, qut.capacity());
}

TEST(MPMC_Queue_FunctionalityTest, FillEmpty)
{
    for (unsigned s = 1; s < 2048; s *= 2)
    {
        es::lockfree::mpmc_queue<int, 0, unsigned, false, false> q0(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, false, true>  q1(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, true, false>  q2(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, true, true>   q3(s);

        testFullEmpty1(q0);
        testFullEmpty1(q1);
        testFullEmpty1(q2);
        testFullEmpty1(q3);
    }
}

template<typename QT>
inline void testFullEmptyIndex(QT &qut)
{
    typename QT::value_type w{0};
    typename QT::value_type r{0};
    typename QT::index_type wi{0};
    typename QT::index_type ri{0};
    unsigned                wloop{0};
    unsigned                rloop{0};

    VERBOSE(std::cerr << __PRETTY_FUNCTION__ << " q capacity: " << qut.capacity() << '\n');
    for (wloop = 0; qut.push(w, wi) && wi == wloop; wloop++) w++;
    for (rloop = 0; qut.peek(r) && r == static_cast<typename QT::value_type>(rloop) && qut.pop(r, ri) &&
                    r == static_cast<typename QT::value_type>(rloop) && ri == rloop;
         rloop++)
        ;
    for (wloop = 0; qut.push(w, wi) && wi == wloop + qut.capacity(); wloop++) w++;
    for (rloop = 0;
         qut.peek(r) && r == static_cast<typename QT::value_type>(rloop + qut.capacity()) && qut.pop(r, ri) &&
         r == static_cast<typename QT::value_type>(rloop + qut.capacity()) && ri == rloop + qut.capacity();
         rloop++)
        ;

    EXPECT_EQ(wloop, rloop);
    EXPECT_EQ(wloop, qut.capacity());
}

TEST(MPMC_Queue_FunctionalityTest, FillEmptyIndex)
{
    for (unsigned s = 1; s < 2048; s *= 2)
    {
        es::lockfree::mpmc_queue<int, 0, unsigned, false, false> q0(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, false, true>  q1(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, true, false>  q2(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, true, true>   q3(s);

        testFullEmptyIndex(q0);
        testFullEmptyIndex(q1);
        testFullEmptyIndex(q2);
        testFullEmptyIndex(q3);
    }
}

template<typename QT>
inline void testFullEmptyKeepN(QT &qut)
{
    typename QT::value_type w{0};
    typename QT::value_type r{0};
    typename QT::index_type wi{0};
    typename QT::index_type ri{0};
    unsigned                wloop{0};
    unsigned                rloop{0};
    const unsigned          z{93};

    VERBOSE(std::cerr << __PRETTY_FUNCTION__ << " q capacity: " << qut.capacity() << '\n');

    for (wloop = 0; qut.push(w, wi) && wi == wloop; wloop++) w++;
    w = z;
    for (wloop = 0; wloop < qut.capacity() && qut.push_keep_n(w, wi) && wi == (wloop + qut.capacity()); wloop++) w++;

    for (rloop = 0;
         rloop < qut.capacity() && qut.peek(r) && r == static_cast<typename QT::value_type>(rloop + z) &&
         qut.pop(r, ri) && r == static_cast<typename QT::value_type>(rloop + z) && ri == (rloop + qut.capacity());
         rloop++)
        EXPECT_EQ(r, rloop + z);

    EXPECT_EQ(wloop, rloop);
    EXPECT_EQ(wloop, qut.capacity());
}

TEST(MPMC_Queue_FunctionalityTest, FillKeepN)
{
    for (unsigned s = 1; s < 2048; s *= 2)
    {
        es::lockfree::mpmc_queue<int, 0, unsigned, false, false> q0(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, false, true>  q1(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, true, false>  q2(s);
        es::lockfree::mpmc_queue<int, 0, unsigned, true, true>   q3(s);

        testFullEmptyKeepN(q0);
        testFullEmptyKeepN(q1);
        testFullEmptyKeepN(q2);
        testFullEmptyKeepN(q3);
    }
}

template<typename QT>
inline void testPushPopOne(QT &qut)
{
    typename QT::value_type r{0};
    typename QT::index_type wi{0};
    typename QT::index_type ri{0};

    VERBOSE(std::cerr << __PRETTY_FUNCTION__ << " q capacity: " << qut.capacity() << '\n');

    EXPECT_TRUE((qut.push(11, wi) && wi == 0 && qut.pop(r, ri) && r == 11 && ri == 0));
    EXPECT_TRUE((qut.push(22, wi) && wi == 1 && qut.pop(r, ri) && r == 22 && ri == 1));
    EXPECT_TRUE((qut.push(33) && qut.pop(r) && r == 33));
    EXPECT_TRUE((qut.push(44) && qut.pop(r) && r == 44));
    EXPECT_TRUE((qut.push(55) && qut.pop(r) && r == 55));
}

TEST(MPMC_Queue_FunctionalityTest, PushOne_f_f)
{
    es::lockfree::mpmc_queue<int, 0, unsigned, false, false> q(8);
    testPushPopOne(q);
}
TEST(MPMC_Queue_FunctionalityTest, PushOne_f_t)
{
    es::lockfree::mpmc_queue<int, 0, unsigned, false, true> q(8);
    testPushPopOne(q);
}
TEST(MPMC_Queue_FunctionalityTest, PushOne_t_f)
{
    es::lockfree::mpmc_queue<int, 0, unsigned, true, false> q(8);
    testPushPopOne(q);
}
TEST(MPMC_Queue_FunctionalityTest, PushOne_t_t)
{
    es::lockfree::mpmc_queue<int, 0, unsigned, true, true> q(8);
    testPushPopOne(q);
}

TEST(MPMC_Queue_FunctionalityTest, different_sizes)
{
    std::array<unsigned, 20> szs{1,         2,         4,         8,          16,         32,        64,
                                 128,       256,       512,       1024,       2048,       4096,      8192,
                                 16 * 1024, 32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024};

    for (auto n : szs)
    {
        VERBOSE(std::cerr << "test Q size: " << n << '\n');
        try
        {
            es::lockfree::mpmc_queue<int, 0, unsigned, false, false> q(n);
            unsigned                                                 x{0};
            for (x = 0; q.push(x); ++x)
                ;
            EXPECT_EQ(x, q.capacity());
        }
        catch (std::exception &e)
        {
            std::cerr << "got exception: " << e.what() << '\n';
        }
    }
}

TEST(MPMC_Queue_FunctionalityTest, PushCharPointer)
{
    es::lockfree::mpmc_queue<const char *, 2, unsigned, false, false> q0(2);
    es::lockfree::mpmc_queue<const char *, 4, unsigned, false, true>  q1(4);
    es::lockfree::mpmc_queue<const char *, 32, unsigned, true, false> q2(32);
    es::lockfree::mpmc_queue<const char *, 64, unsigned, true, true>  q3(64);

    q0.push("Hello");
    q1.push("World");
    q2.push("What's");
    q3.push("Up?");

    const char *a{0}, *b{0}, *c{0}, *d{0};
    q0.pop(a);
    q1.pop(b);
    q2.pop(c);
    q3.pop(d);

    VERBOSE(std::cerr << a << b << c << d << '\n');

    EXPECT_TRUE(a && b && c && d);
    EXPECT_EQ("Hello", a);
    EXPECT_EQ("World", b);
    EXPECT_EQ("What's", c);
    EXPECT_EQ("Up?", d);
}

TEST(MPMC_Queue_FunctionalityTest, PushPopIndex)
{
    using queue_type = es::lockfree::mpmc_queue<int, 0, unsigned, false, false>;

    queue_type::index_type push_index{0};
    queue_type::index_type pop_index{0};

    queue_type q(4);
    int        a{0};

    bool b = q.push(33, push_index) && q.push(33, push_index) && q.push(33, push_index) && q.pop(a, pop_index) &&
             q.pop(a, pop_index) && q.pop(a, pop_index) && pop_index == 2 && push_index == 2;

    ASSERT_TRUE(b);
}

struct Data12
{
    Data12() = default;
    Data12(uint64_t v0, uint16_t v1 = 0, uint16_t v2 = 0) : a(v0), b(v1), c(v2) {}
    uint64_t a;
    uint16_t b, c;
} __attribute__((packed));
static_assert(sizeof(Data12) == 12, "something wrong");

std::ostream &operator<<(std::ostream &os, const Data12 &d12)
{
    os << "data12: a: " << d12.a << " b: " << d12.b << " c: " << d12.c;
    return os;
}

TEST(MPMC_Queue_FunctionalityTest, Data12)
{
    Data12 x{1, 2, 3};

    es::lockfree::mpmc_queue<Data12, 16, uint32_t, false, true> q0{16};

    q0.push(x);

    Data12 y{0, 0, 0};
    q0.pop(y);

    EXPECT_TRUE(y.a == 1 && y.b == 2 && y.c == 3);
}

TEST(hash_signature_tests, IntStringHash)
{
    es::utils::hash_signature h1{};

    es::utils::hash_signature a{1};
    VERBOSE(std::cout << "a: " << a << '\n');
    a(234);
    a.add(456);
    a(456);
    VERBOSE(std::cout << "a: " << a << '\n');
    a(std::string_view{"abcdefghijjklmnop"});
    VERBOSE(std::cout << "a: " << a << '\n');
    a.add(std::string_view{""});
    VERBOSE(std::cout << "a: " << a << '\n');

    a("");
    auto [v, c] = a();
    VERBOSE(std::cout << "v: " << std::hex << v << std::dec << " c: " << c << '\n');

    EXPECT_TRUE(c == 8);  // == 3 && b == 4 && c == 5 && d == 6);
                          //  EXPECT_EQ(1, 1);
}

TEST(hash_signature_tests, HashWrap)
{
    es::utils::hash_signature hs{~0UL};
    auto [v, c] = hs();
    VERBOSE(std::cout << "~0a: " << hs << '\n');
    // test object: state: a5a5a5a5a5a5a5a4 count: 0
    EXPECT_EQ(0xa5a5a5a5a5a5a5a4UL, v);
    EXPECT_EQ(0, c);
    EXPECT_TRUE(hs.get_count() == 0);
}

TEST(MPMC_Queue_FunctionalityTest, HashInputOutput)
{
    es::lockfree::mpmc_queue<uint64_t, 16, uint32_t, true, false> qut(16);

    es::utils::hash_signature h_in{1};
    es::utils::hash_signature h_out{1};

    constexpr uint32_t loops{1000000};
    EXPECT_TRUE(h_in == h_out);

    std::thread qwrite{[&]() {
        for (uint64_t i = 0; i < loops; ++i)
        {
            h_in(i);
            while (!qut.push(i))
                ;
        }
    }};
    std::thread qread{[&]() {
        for (uint32_t i = 0; i < loops; ++i)
        {
            uint64_t v{0};
            while (!qut.pop(v))
                ;
            h_out(v);
        }
    }};
    qwrite.join();
    qread.join();
    VERBOSE(std::cout << "h_out: " << h_out << '\n');
    EXPECT_TRUE(h_in == h_out);
}

class spin_lock
{
    std::atomic_flag _spin_lock = ATOMIC_FLAG_INIT;

public:
    void lock()
    {
        while (_spin_lock.test_and_set())
            ;
    }
    void unlock() { _spin_lock.clear(); }
};

struct Data2
{
    uint64_t _v;
    uint8_t  _hindex;
} __attribute__((packed));
std::ostream &operator<<(std::ostream &os, const Data2 &d)
{
    os << "Data2: _v: " << d._v << " index: " << d._hindex;
    return os;
}

TEST(MPMC_Queue_FunctionalityTest, hash2)
{
    constexpr uint32_t loops{1000000};
    constexpr unsigned PRODUCERS{3};
    constexpr unsigned CONSUMERS{1};  // Current version doesn't work with more than single consumer, as there is not
                                      // order guarentee, after popping the value, that will get the lock.

    es::lockfree::mpmc_queue<Data2, 16, uint32_t, true, false> qut(16);

    std::array<es::utils::hash_signature, PRODUCERS> hin_a;
    std::array<es::utils::hash_signature, PRODUCERS> hout_a;
    std::array<spin_lock, PRODUCERS>                 slocks;

    std::array<std::thread, PRODUCERS> producers;
    std::array<std::thread, CONSUMERS> consumers;

    std::atomic<unsigned> producers_count{PRODUCERS};

    for (auto &p : producers)
        p = std::thread{[&](uint8_t id) {
                            for (uint64_t i = 0; i < loops; ++i)
                            {
                                hin_a[id](i);
                                while (!qut.push(Data2{i, id}))
                                    ;
                            }
                            producers_count--;
                        },
                        (&p - &producers[0])};

    while (qut.empty())
        ;
    for (auto &c : consumers)
        c = std::thread{[&]() {
            while (!qut.empty() || producers_count > 0)
            {
                Data2 d{};
                if (qut.pop(d))
                {
                    std::lock_guard<spin_lock> guard(slocks[d._hindex]);
                    hout_a[d._hindex](d._v);
                }
            }
        }};
    for (auto &p : producers) p.join();
    for (auto &c : consumers) c.join();

    for (unsigned i = 0; i < PRODUCERS; ++i)
        VERBOSE(std::cout << "in[" << i << "]: " << hin_a[i] << " out[" << i << "]: " << hout_a[i] << '\n');

    bool same{true};
    for (unsigned i = 0; i < PRODUCERS; ++i) same &= (hin_a[i] == hout_a[i]);

    EXPECT_TRUE(same);
}

struct Data3
{
    uint32_t _v;
    uint32_t _seq_num;
    uint8_t  _hindex;
} __attribute__((packed));
std::ostream &operator<<(std::ostream &os, const Data3 &d)
{
    os << "Data3: _v: " << d._v << " index: " << d._hindex;
    return os;
}

TEST(MPMC_Queue_FunctionalityTest, hash3)
{
    constexpr uint32_t loops{1000000};
    constexpr unsigned PRODUCERS{4};
    constexpr unsigned CONSUMERS{3};

    es::lockfree::mpmc_queue<Data3, 4, uint32_t, true, false> qut{};

    std::array<es::utils::hash_signature, PRODUCERS> hin_a;
    std::array<es::utils::hash_signature, PRODUCERS> hout_a;
    std::array<spin_lock, PRODUCERS>                 slocks;

    std::array<std::thread, PRODUCERS> producers;
    std::array<std::thread, CONSUMERS> consumers;

    std::atomic<unsigned> producers_count{PRODUCERS};

    for (auto &p : producers)
        p = std::thread{[&](uint8_t id) {
                            for (uint32_t i = 0; i < loops; ++i)
                            {
                                hin_a[id](i * i);
                                while (!qut.push(Data3{i * i, i, id}))
                                    ;
                            }
                            producers_count--;
                        },
                        (&p - &producers[0])};

    while (qut.empty())
        ;
    for (auto &c : consumers)
        c = std::thread{[&]() {
            while (!qut.empty() || producers_count > 0)
            {
                Data3 d{};
                if (qut.pop(d))
                    while (true)
                    {
                        std::lock_guard<spin_lock> guard(slocks[d._hindex]);
                        if (d._seq_num == hout_a[d._hindex].get_count())
                        {
                            hout_a[d._hindex](d._v);
                            break;
                        }
                    }
            }
        }};
    for (auto &p : producers) p.join();
    for (auto &c : consumers) c.join();

    for (unsigned i = 0; i < PRODUCERS; ++i)
        VERBOSE(std::cout << "in[" << i << "]: " << hin_a[i] << " out[" << i << "]: " << hout_a[i] << '\n');

    bool same{true};
    for (unsigned i = 0; i < PRODUCERS; ++i) same &= (hin_a[i] == hout_a[i]);

    EXPECT_TRUE(same);
}

int main(int argc, char **argv)
{
    std::cerr << "Gtest main(" << argc << ", argv) from: " __FILE__ "\n";
    testing::InitGoogleTest(&argc, argv);

    for (auto aindex = 1; aindex < argc; ++aindex)
        if (std::string("--verbose") == argv[aindex]) gl.verbose = true;
    return RUN_ALL_TESTS();
}
