//
// Lockfree, atomic, multi producer, multi consumer queue
//  mpmc queue performance timing utility
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

#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
//
#include <stdlib.h>  // abort()
#include <sys/time.h>
#include <unistd.h>
#include <x86intrin.h>

namespace es::lockfree::tests {

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

template<typename Q>
class QTiming
{
public:
    QTiming(Q& q, unsigned q_depth, unsigned producers_count, unsigned consumers_count, unsigned max_count)
        : _q(q),
          _q_depth(q_depth),
          _from(producers_count),
          _to(producers_count),
          _pop_count(consumers_count),
          _max_count(max_count)
    {
        _producers.resize(producers_count);
        _consumers.resize(consumers_count);
        for (auto& x : _from) x = _max_count;
        for (auto& x : _to) x = 0;
        for (auto& x : _pop_count) x = 0;
    }

    void run();

    void report()
    {
        for (auto& x : _to) std::cout << "to: " << x << '\n';
    }

    Q& _q;

private:
    unsigned                           _q_depth{0};
    std::vector<std::thread>           _producers;
    std::vector<std::thread>           _consumers;
    std::vector<std::atomic<uint64_t>> _from{};
    std::vector<std::atomic<uint64_t>> _to{};
    std::vector<std::atomic<uint64_t>> _pop_count{};
    unsigned long                      _max_count{0};
    std::atomic<unsigned>              _producers_counter{0};
};

template<typename Q>
void QTiming<Q>::run()
{
    auto tsc_start = rdtsc();
    auto tsc_end   = rdtsc();

    std::cout << "tsc diff: " << (tsc_end - tsc_start) << " tsc per milli sec: " << tsc_per_milli() << '\n';

    typename Q::value_type d{17};

    for (unsigned loop = 0; loop < 4; ++loop)
    {
        auto push_tsc_start = rdtsc();
        for (unsigned i = 0; i < _q_depth; ++i) _q.push(d);
        auto push_tsc_end = rdtsc();

        for (unsigned i = 0; i < _q_depth; ++i) _q.pop(d);
        auto pop_tsc_end = rdtsc();
        std::cout << "push cycles: " << (1.0 * (push_tsc_end - push_tsc_start) / _q_depth) << '\n';
        std::cout << "pop  cycles: " << (1.0 * (pop_tsc_end - push_tsc_end) / _q_depth) << '\n';
    }

    std::thread reader{[&]() {
        uint64_t* diffs = new uint64_t[_q_depth + 1];

        typename Q::value_type stsc{1};
        unsigned               n{0};
        while (stsc != 0)
        {
            if (_q.pop(stsc))
            {
                volatile uint64_t tsc = rdtsc();
                diffs[n++]            = tsc - stsc;
            }
        }
        for (unsigned i = 0; i < _q_depth; ++i)
        {
            auto& dt{diffs[i]};
            std::cout << "in q dt: tsc: " << dt << " us: " << (1000.0 * dt / tsc_per_milli()) << '\n';
        }
    }};

    std::thread writer{[&]() {
        for (unsigned i = 0; i < _q_depth; ++i)
        {
            while (!_q.push(rdtsc()))
                ;
            rdtsc();
            rdtsc();
        }
        while (!_q.push(0))
            ;
    }};

    std::cout << "waiting for threads to complete\n";
    reader.join();
    writer.join();

    std::cout << "QTiming run end\n";
}

template<typename Q>
class QBandwidth
{
public:
    QBandwidth(Q& q, unsigned producers_count, unsigned consumers_count, unsigned milliseconds)
        : _q(q), _push_counter(producers_count), _pop_counter(consumers_count), _milli_sec_run(milliseconds)
    {
        _producers.resize(producers_count);
        _consumers.resize(consumers_count);
        for (auto& x : _push_counter) x = 0;
        for (auto& x : _pop_counter) x = 0;
    }

    void run(const std::string& = "");

    Q& _q;

private:
    std::vector<std::thread>           _producers;
    std::vector<std::thread>           _consumers;
    std::vector<std::atomic<uint64_t>> _push_counter{};
    std::vector<std::atomic<uint64_t>> _pop_counter{};
    unsigned long                      _milli_sec_run{0};
    volatile std::atomic<unsigned>     _producers_counter{0};
    volatile std::atomic<unsigned>     _consumers_counter{0};
    std::atomic<unsigned>              _state{0};
};

template<typename Q>
void QBandwidth<Q>::run(const std::string& name)
{
    for (auto& x : _push_counter) x = 0;
    for (auto& x : _pop_counter) x = 0;
    _consumers_counter = 0;
    _producers_counter = 0;
    _state             = 0;

    for (auto& writer : _producers)
    {
        writer = std::thread{[&](unsigned writer_id) {
                                 uint64_t counter{0};
                                 // std::cout << "producer[" << writer_id << "]: started\n";
                                 ++_producers_counter;
                                 while (_state < 1)
                                     ;
                                 while (_state == 1)
                                 {
                                     typename Q::value_type d = counter;
                                     if (_q.push(d)) counter++;
                                 }
                                 _push_counter[writer_id] = counter;
                                 // std::cout << "producer[" << writer_id << "]: ended\n";
                                 --_producers_counter;
                             },
                             (&writer - &_producers[0])};
    }

    for (auto& reader : _consumers)
    {
        reader = std::thread{[&](unsigned reader_id) {
                                 // std::cout << "consumer[" << reader_id << "]: started\n";

                                 uint64_t               counter{0};
                                 typename Q::value_type d{0};
                                 ++_consumers_counter;
                                 while (_state < 1)
                                     ;

                                 while (_state < 3 || _producers_counter > 0 || !_q.empty())
                                 {
                                     while (_q.pop(d)) counter++;
                                 }
                                 _pop_counter[reader_id] = counter;
                                 --_consumers_counter;

                                 // std::cout << "consumer[" << reader_id << "]: ended: " << counter
                                 //          << (_q.empty() ? " Empty OK" : " non empty ERROR") << '\n';
                             },
                             (&reader - &_consumers[0])};
    }

    for (auto& x : _push_counter) x = 0;
    for (auto& x : _pop_counter) x = 0;

    while (_consumers_counter < _consumers.size() || _producers_counter < _producers.size())
        ;
    // std::cout << "sleeping for " << _milli_sec_run << " milliseconds\n";
    auto run_start = rdtsc();
    _state         = 1;
    usleep(1000 * _milli_sec_run);
    _state = 2;

    uint64_t wait_tsc_end = rdtsc() + 1000 * tsc_per_milli();

    while (_producers_counter > 0)
    {
        if (rdtsc() > wait_tsc_end)
        {
            std::cerr << "Producers - too long waiting - error state Q\n" << _q << '\n';
            break;
        }
    }

    _state       = 4;
    wait_tsc_end = rdtsc() + 1000 * tsc_per_milli();
    while (_consumers_counter > 0)
    {
        if (rdtsc() > wait_tsc_end)
        {
            std::cerr << "Consumers - too long waiting error state Q\n" << _q << '\n';
            abort();  // exit(1);
        }
    }
    auto run_end = rdtsc();
    // std::cout << "waiting for threads to complete\n";
    for (auto& w : _producers)
        if (w.joinable()) w.join();
    for (auto& r : _consumers)
        if (r.joinable()) r.join();

    uint64_t total_push{0};
    uint64_t total_pop{0};
    for (auto& x : _push_counter)
    {
        total_push += x;
        // std::cout << "push [" << (&x - &_push_counter[0]) << "]: " << x << '\n';
    }
    for (auto& x : _pop_counter)
    {
        total_pop += x;
        // std::cout << "pop[" << (&x - &_pop_counter[0]) << "]: " << x << '\n';
    }
    std::string emsg{};

    std::cout << name << " push: " << total_push << " pop: " << total_pop << (total_push == total_pop ? "" : " ERROR")
              << " tsc: " << (run_end - run_start)
              << " tsc/op: " << (total_push != 0 ? (run_end - run_start) / total_push : 0) << emsg
              << " push/pop per sec: " << (1000 * total_push * tsc_per_milli() / (run_end - run_start)) << '\n';
    if (total_push != total_pop)
        std::cout << "Q: push/pop difference: " << (total_push - total_pop) << " " << _q << '\n';
}

}  // namespace es::lockfree::tests
