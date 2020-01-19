
#include <iostream>
#include <thread>
//
#include <mpmc_queue.h>
#include <mpmc_queue_pack.h>
#include <sys/time.h>
#include <unistd.h>
#include <x86intrin.h>

uint64_t rdtsc() { return __rdtsc(); }

inline uint64_t tsc_per_milli(bool force = false)
{
    static uint64_t tsc_in_milli{0};

    if (tsc_in_milli && !force) return tsc_in_milli;

    uint64_t ccstart0, ccstart1, ccend0, ccend1;
    timeval  todstart{}, todend{};

    ccstart0 = rdtsc();
    gettimeofday(&todstart, nullptr);
    ccstart1 = rdtsc();
    usleep(10000);  // sleep for 10 milli seconds
    ccend0 = rdtsc();
    gettimeofday(&todend, nullptr);
    ccend1 = rdtsc();

    tsc_in_milli = 1000.0 * ((ccend1 + ccend0) / 2 - (ccstart1 + ccstart0) / 2) /
                   ((todend.tv_sec - todstart.tv_sec) * 1000000UL + todend.tv_usec - todstart.tv_usec);
    return tsc_in_milli;
}

inline double tsc_to_nano(uint64_t tsc_diff) { return (1.0 * tsc_diff / tsc_per_milli()) * 1'000'000; }

template<typename QUT>
uint64_t bw_benchmark(unsigned queue_depth = 32, unsigned P = 3, unsigned C = 3, unsigned N = 1000000)
{
    using queue_pack_type = QUT;  // Queue under test

    using producer_type = typename es::lockfree::producer_accessor<queue_pack_type>::type;
    using consumer_type = typename es::lockfree::consumer_accessor<queue_pack_type>::type;
    using value_type    = typename QUT::value_type;

    std::unique_ptr<queue_pack_type> qpack = std::make_unique<queue_pack_type>(queue_depth);

    std::atomic<uint64_t>  prod_sum{0};
    std::atomic<uint64_t>  cons_sum{0};
    std::atomic<uint64_t>  active_producers{0};
    std::atomic<uint64_t>  active_consumers{0};
    std::atomic<unsigned>  benchmark_state{0};
    std::atomic<uint64_t>* consumers_pops = new std::atomic<uint64_t>[C];

    for (unsigned cons_index = 0; cons_index < C; ++cons_index) consumers_pops[cons_index] = 0;

    auto producer = [&](int producer_id) {
        ++active_producers;
        while (benchmark_state)
            ;  // wait to start.
        producer_type pro{*qpack};
        uint64_t      m{0};
        for (unsigned x = 0; x < N; ++x)
        {
            typename queue_pack_type::value_type v = x + 10 * N * producer_id;
            while (!pro.push(v))
                ;
            m += v;
        }
        prod_sum += m;
        --active_producers;
    };
    std::vector<std::thread> producers;
    producers.resize(P);
    for (auto& p : producers) p = std::thread{producer, (&p - &producers[0])};

    while (active_producers != P)
        ;

    auto consumer = [&](int consumer_id) {
        consumer_type con{*qpack};
        uint64_t      m{0};
        value_type    v{0};
        uint64_t      n{0};
        ++active_consumers;
        do
        {
            while (con.pop(v))
            {
                m += v;
                ++n;
            }
        } while (active_producers);

        cons_sum += m;
        consumers_pops[consumer_id] = n;
        --active_consumers;
    };
    std::vector<std::thread> consumers;
    consumers.resize(C);
    for (auto& c : consumers) c = std::thread{consumer, (&c - &consumers[0])};

    while (active_consumers != C)
        ;

    uint64_t t0     = rdtsc();
    benchmark_state = 1;
    while (active_consumers != 0)
        ;
    t0 = rdtsc() - t0;

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    std::cout << (cons_sum && cons_sum == prod_sum ? "OK" : "ERROR") << " " << prod_sum << " " << cons_sum << " "
              << __PRETTY_FUNCTION__ << " " << (0.000001 * tsc_to_nano(t0)) << "ms\n";
    uint64_t total_pops{0};
    for (unsigned cons_index = 0; cons_index < C; ++cons_index)
    {
        auto& cp{consumers_pops[cons_index]};
        std::cout << " consumer[" << (&cp - &consumers_pops[0]) << "]: " << cp << '\n';
        total_pops += cp;
    }
    std::cout << "total push/pops: " << total_pops << " nano seconds per push/pop: " << tsc_to_nano(t0 / total_pops)
              << " " << (t0 / total_pops) << "cc" << '\n';
    delete[] consumers_pops;
    return t0;
}

int main()
{
    using namespace es::lockfree;

    {
        auto simple_q_time = bw_benchmark<mpmc_queue<uint32_t, 0, uint32_t, false, false>>(32, 1, 1);
        auto q_pack_time =
            bw_benchmark<mpmc_queue_pack<mpmc_queue<uint32_t, 0, uint32_t, false, false>, 3, 10>>(32, 1, 1);
        std::cout << "speed up: " << (1.0 * simple_q_time / q_pack_time) << '\n';
    }
    {
        auto simple_q_time = bw_benchmark<mpmc_queue<uint32_t, 0, uint32_t, false, false>>(32);
        auto q_pack_time   = bw_benchmark<mpmc_queue_pack<mpmc_queue<uint32_t, 0, uint32_t, false, false>, 3, 10>>(32);
        std::cout << "speed up: " << (1.0 * simple_q_time / q_pack_time) << '\n';
    }
    {
        auto simple_q_time = bw_benchmark<mpmc_queue<uint32_t, 32, uint32_t, false, false>>(32);
        auto q_pack_time   = bw_benchmark<mpmc_queue_pack<mpmc_queue<uint32_t, 32, uint32_t, false, false>, 3, 10>>(32);
        std::cout << "speed up: " << (1.0 * simple_q_time / q_pack_time) << '\n';
    }
    {
        auto simple_q_time = bw_benchmark<mpmc_queue<uint32_t, 32, uint32_t, false, false>>(32, 2, 2);
        auto q_pack_time =
            bw_benchmark<mpmc_queue_pack<mpmc_queue<uint32_t, 32, uint32_t, false, false>, 4, 16>>(32, 2, 2);
        std::cout << "speed up: " << (1.0 * simple_q_time / q_pack_time) << '\n';
    }
    {
        auto simple_q_time = bw_benchmark<mpmc_queue<uint32_t, 32, uint32_t, false, false>>(32, 1, 1);
        auto q_pack_time =
            bw_benchmark<mpmc_queue_pack<mpmc_queue<uint32_t, 32, uint32_t, false, false>, 4, 16>>(32, 1, 1);
        std::cout << "speed up: " << (1.0 * simple_q_time / q_pack_time) << '\n';
    }
    {
        auto simple_q_time = bw_benchmark<mpmc_queue<uint64_t, 0, uint32_t, true, true>>();
        auto q_pack_time   = bw_benchmark<mpmc_queue_pack<mpmc_queue<uint64_t, 0, uint32_t, true, true>, 4, 16>>();
        std::cout << "speed up: " << (1.0 * simple_q_time / q_pack_time) << '\n';
    }
    {
        auto simple_q_time = bw_benchmark<mpmc_queue<uint64_t, 0, uint32_t, false, false>>();
        auto q_pack_time   = bw_benchmark<mpmc_queue_pack<mpmc_queue<uint64_t, 0, uint32_t, false, false>, 4, 16>>();
        std::cout << "speed up: " << (1.0 * simple_q_time / q_pack_time) << '\n';
    }
    return 0;
}
