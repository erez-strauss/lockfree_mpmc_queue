//
// Lockfree, atomic, multi producer, multi consumer queue
//  test / example usage application
//
// MIT License
//
// Copyright (c) 2019 Erez Strauss, erez@erezstrauss.com
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

#include "mpmc_queue.h"
#include "mpmc_queue_timing.h"

#include <atomic>
#include <functional>
#if defined(COMPARE_BOOST) && COMPARE_BOOST == 1
#include <boost/lockfree/queue.hpp>
#endif
#include <thread>
#include <vector>

#if defined(COMPARE_BOOST) && COMPARE_BOOST == 1
namespace boost::lockfree {
// This operator is defined as it is required for the timing benchmark, as it tries to print a Queue state in case of
// error.
template<typename DT, size_t QD>
inline std::ostream& operator<<(std::ostream& os, boost::lockfree::queue<DT, boost::lockfree::capacity<QD>>&)
{
    os << " nothing to print for boost queue";
    return os;
}
}  // namespace boost::lockfree
#endif

// template on data size and index size and q-depth
// q depth: 1,2 4, 32, 256, 1024, 8192
// data size: 1 2 4 8, 14, 15b (for small q depth)
// index_type 1 2 4 8 bytes

template<typename DT, unsigned Q_DEPTH, typename IT = uint32_t>
void test_bandwidth_mpmc_queue(size_t qdepth = Q_DEPTH, unsigned producers = 1, unsigned consumers = 1,
                               unsigned millis = 1000, bool test_boostq = false)
{
    using QUT1A = es::lockfree::mpmc_queue<DT, Q_DEPTH, IT, false, false>;
    using QUT1B = es::lockfree::mpmc_queue<DT, Q_DEPTH, IT, false, true>;
    using QUT1C = es::lockfree::mpmc_queue<DT, Q_DEPTH, IT, true, false>;
    using QUT1D = es::lockfree::mpmc_queue<DT, Q_DEPTH, IT, true, true>;

    if (test_boostq)
    {
#if defined(COMPARE_BOOST) && COMPARE_BOOST == 1
        using QUT2 = boost::lockfree::queue<DT, boost::lockfree::capacity<Q_DEPTH>>;
        QUT2 boost_q{};

        std::cout << "Q BW: data size: " << sizeof(DT) << " -     -     -"
                  << " capacity: " << std::setw(3) << qdepth << " producers: " << producers
                  << " conuumers: " << consumers << " for: " << millis << "ms ";

        es::lockfree::tests::QBandwidth<QUT2> bwt2(boost_q, producers, consumers, millis);
        bwt2.run("boost:lf:queue");
#else
        std::cerr << "q_bandwidth was built without comparison to boost::lockfree::queue\n";
        exit(1);
#endif
    }
    else
    {
        std::cout << "Q BW: data size: " << sizeof(DT) << " index size: " << sizeof(IT) << " capacity: " << std::setw(3)
                  << qdepth << " producers: " << producers << " consumers: " << consumers << " for: " << millis
                  << "ms ";
        {
            QUT1A qa{qdepth};
            es::lockfree::tests::QBandwidth<QUT1A> bwt1a(qa, producers, consumers, millis);
            bwt1a.run("mpmc_queue<ff>");
        }
        std::cout << "Q BW: data size: " << sizeof(DT) << " index size: " << sizeof(IT) << " capacity: " << std::setw(3)
                  << qdepth << " producers: " << producers << " consumers: " << consumers << " for: " << millis
                  << "ms ";
        {
            QUT1B qb{qdepth};
            es::lockfree::tests::QBandwidth<QUT1B> bwt1b(qb, producers, consumers, millis);
            bwt1b.run("mpmc_queue<ft>");
        }
        std::cout << "Q BW: data size: " << sizeof(DT) << " index size: " << sizeof(IT) << " capacity: " << std::setw(3)
                  << qdepth << " producers: " << producers << " consumers: " << consumers << " for: " << millis
                  << "ms ";
        {
            QUT1C qc{qdepth};
            es::lockfree::tests::QBandwidth<QUT1C> bwt1c(qc, producers, consumers, millis);
            bwt1c.run("mpmc_queue<tf>");
        }
        std::cout << "Q BW: data size: " << sizeof(DT) << " index size: " << sizeof(IT) << " capacity: " << std::setw(3)
                  << qdepth << " producers: " << producers << " consumers: " << consumers << " for: " << millis
                  << "ms ";
        {
            QUT1D qd{qdepth};
            es::lockfree::tests::QBandwidth<QUT1D> bwt1d(qd, producers, consumers, millis);
            bwt1d.run("mpmc_queue<tt>");
        }
    }
}

struct Data12
{
    Data12(uint64_t = 0) {}

    uint8_t _data[12];
};
struct Data14
{
    Data14(uint64_t = 0) {}

    uint8_t _data[14];
};
struct Data15
{
    Data15(uint64_t = 0) {}

    uint8_t _data[15];
};

inline std::ostream& operator<<(std::ostream& os, const uint8_t& u8)
{
    os << (unsigned)u8;
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const Data12&)
{
    os << " data12 ";
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const Data14&)
{
    os << " data14 ";
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const Data15&)
{
    os << " data15 ";
    return os;
}

std::array<std::pair<std::string, std::function<void()>>, 5> tests{
    std::make_pair("bandwidth test compare to boost::lockfree::queue 4 btyes data size",
                   []() { test_bandwidth_mpmc_queue<uint32_t, 32>(); }),
    std::make_pair("bandwidth test compare to boost::lockfree::queue 8 btyes data size",
                   []() { test_bandwidth_mpmc_queue<uint64_t, 32>(); }),
    std::make_pair("bandwidth test compare to boost::lockfree::queue 12 btyes data size",
                   []() { test_bandwidth_mpmc_queue<Data12, 32>(); })};

void list_tests()
{
    for (auto& t : tests) std::cout << (1 + (&t - &tests[0])) << " " << t.first << '\n';
}

class TestApp
{
public:
    TestApp(int argc, char** argv) { args(argc, argv); }

    template<unsigned>
    void runD();

    void run();

    void usage();

private:
    const char* _argv0;
    unsigned _test{0};
    unsigned _q_depth{32};
    unsigned _readers{2};
    unsigned _writers{2};
    unsigned _data_bytes{8};
    unsigned _index_bytes{4};
    unsigned _millis{1000};
    constexpr static inline std::array<unsigned, 5> compile_sizes{2, 8, 128, 512, 4096};
    bool _boost_q_flag{false};
    bool _inplace{true};

    void args(int argc, char** argv);

    template<typename DT, typename IT>
    inline void run_queue_test();
};

void TestApp::args(int argc, char** argv)
{
    int opt;
    _argv0 = argv[0];
    while ((opt = getopt(argc, argv, "m:n:p:c:d:w:W:habt:TD")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage();
                exit(0);
                break;
            case 'b':
#if defined(COMPARE_BOOST) && COMPARE_BOOST == 1
                _boost_q_flag = true;
#else
                std::cerr << "q_bandwidth was built without comparison to boost::lockfree::queue\n";
                exit(1);
#endif
                break;
            case 'm':
                _millis = atoi(optarg);
                break;
            case 't':
                _test = atoi(optarg);
                break;
            case 'T':
                list_tests();
                exit(0);
                break;
            case 'p':
                _writers = atoi(optarg);
                break;
            case 'c':
                _readers = atoi(optarg);
                break;
            case 'd':
            {
                int x{atoi(optarg)};
                if (x > 0)
                    _q_depth = x;
                else
                    std::cerr << "worng depth value: " << x << " '" << optarg << "\n";
                break;
            }
            case 'w':
                _index_bytes = atoi(optarg);
                break;
            case 'W':
                _data_bytes = atoi(optarg);
                break;
            case 'D':
                _inplace = false;
                break;
            default:
                std::cerr << "unknown option: " << opt << "\n";
                usage();
                exit(1);
        }
    }
}

template<unsigned TEST_Q_DEPTH>
void TestApp::runD()
{
    switch (_index_bytes)
    {
        case 4:
            switch (_data_bytes)
            {
                case 1:
                    test_bandwidth_mpmc_queue<uint8_t, TEST_Q_DEPTH, uint32_t>(_q_depth, _writers, _readers, _millis,
                                                                               _boost_q_flag);
                    break;
                case 2:
                    test_bandwidth_mpmc_queue<uint16_t, TEST_Q_DEPTH, uint32_t>(_q_depth, _writers, _readers, _millis,
                                                                                _boost_q_flag);
                    break;
                case 4:
                    test_bandwidth_mpmc_queue<uint32_t, TEST_Q_DEPTH, uint32_t>(_q_depth, _writers, _readers, _millis,
                                                                                _boost_q_flag);
                    break;
                case 8:
                    test_bandwidth_mpmc_queue<uint64_t, TEST_Q_DEPTH, uint32_t>(_q_depth, _writers, _readers, _millis,
                                                                                _boost_q_flag);
                    break;
                case 12:
                    test_bandwidth_mpmc_queue<Data12, TEST_Q_DEPTH, uint32_t>(_q_depth, _writers, _readers, _millis,
                                                                              _boost_q_flag);
                    break;
                default:
                    std::cerr << "Error: config - data width [1,2,4,8,12] bytes\n";
                    exit(1);
            }
            break;
        case 8:
            switch (_data_bytes)
            {
                case 1:
                    test_bandwidth_mpmc_queue<uint8_t, TEST_Q_DEPTH, uint64_t>(_q_depth, _writers, _readers, _millis,
                                                                               _boost_q_flag);
                    break;
                case 2:
                    test_bandwidth_mpmc_queue<uint16_t, TEST_Q_DEPTH, uint64_t>(_q_depth, _writers, _readers, _millis,
                                                                                _boost_q_flag);
                    break;
                case 4:
                    test_bandwidth_mpmc_queue<uint32_t, TEST_Q_DEPTH, uint64_t>(_q_depth, _writers, _readers, _millis,
                                                                                _boost_q_flag);
                    break;
                case 8:
                    test_bandwidth_mpmc_queue<uint64_t, TEST_Q_DEPTH, uint64_t>(_q_depth, _writers, _readers, _millis,
                                                                                _boost_q_flag);
                    break;
                default:
                    std::cerr << "Error: config - data width [1,2,4,8] bytes\n";
                    exit(1);
            }
            break;
        default:
            std::cerr << "Error: config - index width [4,8] bytes\n";
            exit(1);
    };
}

void TestApp::run()
{
    if (_test)
    {
        if (_test > tests.size())
        {
            std::cerr << "Test index out of range, use -T to list available tests\n";
            usage();
            exit(1);
        }
        std::cout << tests[_test - 1].first << '\n';
        tests[_test - 1].second();
    }
    else
    {
        if (_inplace)
        {
            switch (_q_depth)
            {
                case 1:
                    runD<1>();
                    break;
                case 2:
                    runD<2>();
                    break;
                case 16:
                    runD<16>();
                    break;
                case 64:
                    runD<64>();
                    break;
                default:
                    runD<0>();  // Allocated by Queue constructor, not inplace.
            }
        }
        else
        {
            runD<0>();  // Allocated by Queue constructor, not inplace.
        }
    }
}

void TestApp::usage()
{
    std::cout << "usage: " << _argv0
              << " [-h][-a][-p P][-c C][-d D][-m MS]"
                 "\n -m -- time in milliseconds"
                 "\n -b -- run tests on boost::lockfree::queue"
                 "\n P - number of producer threads,"
                 "\n C - number of consumer threads,"
                 "\n -d - queue's depth, capacity"
                 "\n -w index bytes [1,2,4,8]"
                 "\n -W data size in bytes [1,2,4,8,12,14,15]"
                 "\n";
}

int main(int argc, char** argv)
{
    try
    {
        TestApp test(argc, argv);
        test.run();
        return 0;
    }
    catch (std::invalid_argument const& ia)
    {
        std::cerr << "invalid argument: " << ia.what() << '\n';
    }
    catch (std::exception const& e)
    {
        std::cerr << "exception: " << e.what() << '\n';
    }
    std::cerr << "error exit(1)\n";
    exit(1);
}
