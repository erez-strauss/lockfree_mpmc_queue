//
// Lockfree, atomic, multi producer, multi consumer queue - between processes
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

#include <mpmc_queue.h>
//
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace es::lockfree {

constexpr const uint32_t ShareMPMCQueue{0x0BadBadB};

struct shared_file_header
{
    uint32_t signature{ShareMPMCQueue};
    uint32_t hdr_size{0};
    uint64_t q_start;
    uint64_t q_size_elements;
    uint64_t q_size_bytes;
    std::atomic<int32_t> producer_count;
    std::atomic<int32_t> consumer_count;
    uint32_t producers[16];
    uint32_t consumers[16];
};

template<typename Q>
class shared_mpmc_queue
{
    class producer
    {
    public:
        producer(shared_mpmc_queue& sq) : _shared_q(sq) { _shared_q.producers_inc(); }

        bool push(typename Q::value_type d) { return _shared_q._qp->push(d); }
        bool push(typename Q::value_type d, typename Q::index_type& i) { return _shared_q._qp->push(d, i); }
        bool push_keep_n(typename Q::value_type d) { return _shared_q._qp->push_keep_n(d); }
        bool push_keep_n(typename Q::value_type d, typename Q::index_type& i)
        {
            return _shared_q._qp->push_keep_n(d, i);
        }

        ~producer() { _shared_q.producers_dec(); }

    private:
        shared_mpmc_queue& _shared_q;
    };
    class consumer
    {
    public:
        consumer(shared_mpmc_queue& sq) : _shared_q(sq) { _shared_q.consumers_inc(); }

        bool pop(typename Q::value_type& d) { return _shared_q._qp->pop(d); }
        bool pop(typename Q::value_type& d, typename Q::index_type& i) { return _shared_q._qp->pop(d, i); }

        ~consumer() { _shared_q.consumers_dec(); }

    private:
        shared_mpmc_queue& _shared_q;
    };

public:
    shared_mpmc_queue(const char* fname)
    {
        int fd = open(fname, O_CREAT | O_RDWR | O_CLOEXEC, 0666);
        if (fd < 0)
        {
            std::cerr << "Error: failed to open/create '" << fname << "': " << strerror(errno) << '\n';
            exit(1);
        }
        shared_file_header header;

        int r = read(fd, &header, sizeof(header));
        if (sizeof(header) != r)
        {
            // create new queu
            header.signature = ShareMPMCQueue;
            header.hdr_size = sizeof(shared_file_header);
            header.q_start = 4096;
            header.q_size_elements = Q::size_n();
            header.q_size_bytes = sizeof(Q);
            header.producer_count = 0;
            header.consumer_count = 0;
            ftruncate(fd, 4096 + sizeof(Q));
            write(fd, &header, sizeof(header));
            _base = mmap(nullptr, 4096 + sizeof(Q), PROT_WRITE, MAP_SHARED, fd, 0);
            _header = (shared_file_header*)_base;
            _qp = new ((void*)((uint64_t)_base + 4096)) Q(Q::size_n());
        }
        else if ((ShareMPMCQueue == header.signature) && (sizeof(shared_file_header) == header.hdr_size) &&
                 (4096 == header.q_start) && Q::size_n() == header.q_size_elements && sizeof(Q) == header.q_size_bytes)
        {
            _base = mmap(nullptr, 4096 + sizeof(Q), PROT_WRITE, MAP_SHARED, fd, 0);
            _header = (shared_file_header*)_base;
            _qp = reinterpret_cast<Q*>((void*)((uint64_t)_base + 4096));
            std::cout << "Attached successfully to shared q: '" << fname << "'\nq: " << *_qp << '\n';
        }
        else
        {
            std::cerr << "Error: shared q file exists but not compatible\n";
            exit(1);
        }
    }

    auto get_producer() { return producer(*this); }

    auto get_consumer() { return consumer(*this); }

    size_t get_producers_count() { return _header->producer_count; }

    size_t get_consumers_count() { return _header->consumer_count; }

    void producers_inc() { _header->producer_count++; }
    void producers_dec() { _header->producer_count--; }
    void consumers_inc() { _header->consumer_count++; }
    void consumers_dec() { _header->consumer_count--; }

    Q* _qp{nullptr};

private:
    int _fd{-1};
    shared_file_header* _header{nullptr};
    void* _base{nullptr};
};
}  // namespace es::lockfree
