//
// Lockfree, atomic, multi producer, multi consumer queue
//
// MIT License
//
// Copyright (c) 2019-2022 Erez Strauss, erez@erezstrauss.com
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

namespace es::lockfree {

template<template<typename, size_t, typename, bool, bool> class Q, typename T, size_t N = 0, typename IndexT = uint32_t,
         template<typename...> class P = std::unique_ptr, bool lazy_push = false, bool lazy_pop = false>
class pointer_mpmc_queue
{
    using ul_type      = P<T>;
    using pointer_type = typename ul_type::pointer;
    using index_type   = IndexT;

    static_assert(sizeof(pointer_type) == sizeof(void*), "mismatch type size");
    static_assert(sizeof(ul_type) == sizeof(void*), "mismatch type size");

public:
    pointer_mpmc_queue(unsigned sz) : _queue(sz) {}
    ~pointer_mpmc_queue()
    {
        pointer_type nv{};
        while (_queue.pop(nv))
        {
            ul_type up{nv};
        }
    }

    bool push(ul_type&& v)
    {
        if (_queue.push(v.get()))
        {
            v.release();
            return true;
        }
        return false;
    }

    bool pop(ul_type& v)
    {
        pointer_type nv{};
        if (_queue.pop(nv))
        {
            v.reset(nv);
            return true;
        }
        return false;
    }

    bool push(ul_type&& v, index_type& i)
    {
        if (_queue.push(v.get(), i))
        {
            v.release();
            return true;
        }
        return false;
    }

    bool pop(ul_type& v, index_type& i)
    {
        pointer_type nv{};
        if (_queue.pop(nv, i))
        {
            v.reset(nv);
            return true;
        }
        return false;
    }

    bool empty() { return _queue.empty(); }
    bool empty() const { return _queue.empty(); }

    auto size() { return _queue.size(); }
    auto size() const { return _queue.size(); }

    auto capacity() const { return _queue.capacity(); }

private:
    Q<pointer_type, N, IndexT, lazy_push, lazy_pop> _queue;
};

}  // namespace es::lockfree
