//
// Lockfree, atomic, multi producer, multi consumer queue pack
//
// MIT License
//
// Copyright (c) 2019,2020 Erez Strauss, erez@erezstrauss.com
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

#include <mpmc_queue.h>

#include <vector>

namespace es::lockfree {

// Q - the basic mpmc queue to hold
// G - how many queues are in a group/pack of mpmc_queues.
// K - every how many successful pop, do a scan of the other queues

template<typename Q, unsigned G, unsigned K>
class mpmc_queue_pack
{
public:
    using value_type = typename Q::value_type;
    using index_type = typename Q::index_type;

    mpmc_queue_pack(unsigned qsz)
    {
        _queues.resize(G);
        for (auto& up : _queues) up = std::make_unique<Q>(qsz);
    }
    class producer
    {
    public:
        producer(mpmc_queue_pack& qp)
            : _queue_pack(qp), _queue_index(qp._writers_count++ % G), _write_queue(*qp._queues[_queue_index])
        {
        }

        auto push(value_type d) { return _write_queue.push(d); }
        auto push(value_type d, index_type& ir) { return _write_queue.push(d, ir); }

    private:
        mpmc_queue_pack& _queue_pack;
        unsigned         _queue_index;
        Q&               _write_queue;
    };

    class consumer
    {
    public:
        consumer(mpmc_queue_pack& qp)
            : _queue_pack(qp),
              _read_queue_index(qp._readers_count++ % G),
              _read_queue(&*qp._queues[_read_queue_index]),
              _pop_count(0)
        {
        }

        bool pop(value_type& d)
        {
            if (_read_queue->pop(d))
            {
                if (++_pop_count >= K)
                {
                    _pop_count        = 0;
                    _read_queue_index = (_read_queue_index + 1) % G;
                    _read_queue       = &*_queue_pack._queues[_read_queue_index];
                }
                return true;
            }
            _pop_count = 0;
            for (unsigned k = 1; k < G; ++k)
            {
                _read_queue_index = (_read_queue_index + 1) % G;
                _read_queue       = &*_queue_pack._queues[_read_queue_index];
                if (_read_queue->pop(d))
                {
                    return true;
                }
            }
            return false;
        }

    private:
        mpmc_queue_pack& _queue_pack;
        unsigned         _read_queue_index{G};
        Q*               _read_queue{nullptr};
        unsigned         _pop_count{0};
    };

private:
    std::atomic<uint64_t> _writers_count{0};
    std::atomic<uint64_t> _readers_count{0};

    std::vector<std::unique_ptr<Q>> _queues;

    friend class producer;
    friend class consumer;
};

template<typename QT>
struct producer_accessor;
template<typename QT>
struct consumer_accessor;

template<typename Q, unsigned G, unsigned K>
struct producer_accessor<mpmc_queue_pack<Q, G, K>>
{
    using type = typename mpmc_queue_pack<Q, G, K>::producer;
};

template<typename Q, unsigned G, unsigned K>
struct consumer_accessor<mpmc_queue_pack<Q, G, K>>
{
    using type = typename mpmc_queue_pack<Q, G, K>::consumer;
};

template<typename Q>
struct producer_accessor
{
    using type = Q&;
};

template<typename Q>
struct consumer_accessor
{
    using type = Q&;
};

}  // namespace es::lockfree
