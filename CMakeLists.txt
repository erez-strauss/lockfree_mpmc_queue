cmake_minimum_required(VERSION 3.14)
project(mpmc_queue)

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_FLAGS "-DCOMPARE_BOOST=0 -std=c++17 -W -Wall -Wshadow -Wpedantic -ggdb3 -O3 -mcx16 " )

include_directories(.)
include_directories(src)

add_executable(gtest_mpmc_queue
        src/gtest_mpmc_queue.cpp
        mpmc_queue.h
        )
target_link_libraries(gtest_mpmc_queue LINK_PRIVATE pthread gtest_main gtest)

add_executable(shared_mpmc_queue
        src/shared_q_test.cpp
        mpmc_queue.h
        shared_mpmc_queue.h
        )
target_link_libraries(shared_mpmc_queue LINK_PRIVATE pthread)

add_executable(q_bandwidth
        src/q_bandwidth.cpp
        mpmc_queue.h
        src/mpmc_queue_timing.h
        )
target_link_libraries(q_bandwidth LINK_PRIVATE pthread)

add_executable(test_uniqptr_q
        src/test_uniqptr_q.cpp
        mpmc_queue.h
        )
target_link_libraries(test_uniqptr_q LINK_PRIVATE pthread)
