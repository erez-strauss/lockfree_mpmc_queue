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

int main()
{
    constexpr unsigned                     N{100};
    es::lockfree::mpmc_queue<unsigned, 32> q{};
    unsigned                               prod_sum{0};
    unsigned                               cons_sum{0};

    std::thread prod{[&]() {
        for (unsigned x = 0; x < N; ++x)
        {
            while (!q.push(x))
                ;
            prod_sum += x;
        }
    }};
    std::thread cons{[&]() {
        unsigned v{0};
        for (unsigned x = 0; x < N; ++x)
        {
            while (!q.pop(v))
                ;
            cons_sum += v;
        }
    }};
    prod.join();
    cons.join();
    std::cout << (cons_sum && cons_sum == prod_sum ? "OK" : "ERROR") << " " << cons_sum << '\n';

    return 0;
}
