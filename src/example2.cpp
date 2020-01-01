//
// Lockfree, atomic, multi producer, multi consumer queue
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

#include <mpmc_queue.h>

#include <iostream>
#include <thread>
#include <vector>

int main()
{
    es::lockfree::mpmc_queue<unsigned> q{32};
    constexpr unsigned                 N{1000000};
    constexpr unsigned                 P{2};
    std::atomic<uint64_t>              prod_sum{0};
    std::atomic<uint64_t>              cons_sum{0};

    auto producer = [&]() {
        uint64_t m{0};
        for (unsigned x = 0; x < N; ++x)
        {
            while (!q.push(x))
                ;
            m += x;
        }
        prod_sum += m;
    };
    std::vector<std::thread> producers;
    producers.resize(P);
    for (auto& p : producers) p = std::thread{producer};

    auto consumer = [&]() {
        uint64_t m{0};
        unsigned v{0};
        for (unsigned x = 0; x < N; ++x)
        {
            while (!q.pop(v))
                ;
            m += v;
        }
        cons_sum += m;
    };
    std::vector<std::thread> consumers;
    consumers.resize(P);
    for (auto& c : consumers) c = std::thread{consumer};

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();
    std::cout << (cons_sum && cons_sum == prod_sum ? "OK" : "ERROR") << " " << cons_sum << '\n';

    return 0;
}
