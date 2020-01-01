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

#include <shared_mpmc_queue.h>

int main(int argc, char** argv)
{
    es::lockfree::shared_mpmc_queue<es::lockfree::mpmc_queue<unsigned, 128, unsigned>> shared_q("/dev/shm/mpmc_q000");

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'p')
    {
        auto prod = shared_q.get_producer();
        prod.push(123);
        prod.push(234);
        prod.push(345);
        if (argc > 2) sleep(60);
    }
    else if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'c')
    {
        auto     cons = shared_q.get_consumer();
        unsigned value{};
        while (true)
        {
            if (cons.pop(value))
            {
                std::cout << "value: " << value << '\n';
            }
            else if (shared_q.get_producers_count() <= 0)
                break;
        }
    }
    else
        std::cout << "Shared Q: " << *shared_q._qp << '\n';

    return 0;
}
