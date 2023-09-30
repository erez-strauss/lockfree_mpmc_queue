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

// Simple hash value and counts
// add("aa") != add("a").add("a") by design and requirements

#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>

namespace es::utils {

class hash_signature
{
public:
    using value_type = uint64_t;
    using count_type = uint64_t;

    explicit hash_signature(value_type seed = 0);

    hash_signature& add(value_type v);
    hash_signature& operator()(value_type v) { return add(v); }

    hash_signature& add(std::string_view sv);
    hash_signature& operator()(std::string_view sv) { return add(sv); }

    std::pair<value_type, count_type> state();
    std::pair<value_type, count_type> operator()() { return state(); }

    count_type get_count() const { return _count; }

    std::ostream& print(std::ostream& os) const
    {
        os << "test object: state: " << std::hex << _state << " count: " << std::dec << _count;
        return os;
    }

    bool operator==(const hash_signature& o) const { return _state == o._state && _count == o._count; }

private:
    value_type _state{};
    count_type _count{0};
};

std::ostream& operator<<(std::ostream& os, const hash_signature& o) { return o.print(os); }

hash_signature::hash_signature(value_type seed) : _state(seed + 0xA5A5A5A5A5A5A5A5UL), _count(0) {}

std::pair<hash_signature::value_type, uint64_t> hash_signature::state() { return {_state, _count}; }

hash_signature& hash_signature::add(value_type v)
{
    if (__builtin_popcount(v) < 20) v += 0xA500A500A5A5;
    _state = (v + _state) * ((v << 3) + (_state << 1) + 1);
    _count++;
    return *this;
}

hash_signature& hash_signature::add(std::string_view sv)
{
    if (sv.size() == 0)
    {
        add(0);
        return *this;
    }
    union
    {
        value_type    _value;
        unsigned char _buffer[sizeof(value_type)];
    };
    while (sv.size() > 0)
    {
        _value = 0;
        memcpy(_buffer, sv.begin(), (sv.size() > sizeof(_buffer) ? sizeof(_buffer) : sv.size()));
        add(_value);
        if (sv.size() > sizeof(_buffer))
            sv = {sv.begin() + sizeof(_buffer), sv.size() - sizeof(_buffer)};
        else
            break;
    }
    return *this;
}

}  // namespace es::utils
