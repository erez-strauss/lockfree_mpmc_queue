
#include <mpmc_queue.h>

#include <iostream>
#include <thread>

int main()
{
    constexpr unsigned N{100};
    es::lockfree::mpmc_queue<unsigned, 32> q{};
    unsigned prod_sum{0};
    unsigned cons_sum{0};

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
