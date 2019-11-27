# mpmc_queue
atomic, lockfree, multi producer, multi consumer queue

## Available at
http://github.com/erez-strauss/lockfree_mpmc_queue/

## The queue requirements

* a header only, lockfree, multi producer, multi consumer queue
* trivial data type, no need for move semantics
* no memory allocation, other than initialization
* none-blocking - no two operation that must be completed by a single producer or consumer
* single atomic operation - on _array entry define success of push()/pop()
* fixed size queue - at run time (as constructor parameter) or compile time (as template parameter)
* fifo - default, or keep last N elements when using push_keep_n()
* access from multi threads as well as multi processes

## Design:

* fixed size array of entries, array size must be power of 2
* the _array entry content is accessed and updated using atomic operation
* maximum size of entry is 16 bytes, in order to guarantee atomic compare and exchange
* each entry is in a separate cacheline - prevents producers and consumers contention.
* _array entries contain sequence number and the user data
* all change operations are done using compare_exchange, both for entries and indexes
* atomic write / read indexes
* push/pop operation are defined successful, once the array entry has changed successfully
* updating the write/read indexes is cooperative
* producers never access the read index
* consumers never access the write index
* push/pop operation sequence number is available to user
* entry has two states: empty or full - indicate by the low bit of the _seq entry's field
* empty entry contain sequence number of the next write
* push(T d) loads the write index, look at the entry in the buffer,if it contains the value of the index,and  entry is
 empty then attempt compare and exchange with a new value which includes the same sequence number with indication that the entry is full.
* pop(T& d) loads the read index, load the entry from the array, if the entry has the read sequence number and is full,
 do compare and exchange with a new empty entry value for the next write operation on the same entry
* lazy push/pop are available as template parameters, where increasing the respective index is left to other producer , consumer to complete
* peek(T& d) and peek(data_type&, index_tyep&) provide the current queue's head, the next value to be pop()ed
* no pointers internally, only indexes, opens to access from multiple processes, see shared_mpmc_queue.h
* with small enough items and indexes, can use compare_exchange operations on 8 bytes
* sequenced operations - each pop/push has an operation sequence number, successful push()/pop() have sequence numbers which are accessible to the user through push(value_type, index_type&) and pop(vale_type&, index_type&)

## Usage:


```
es::lockfree::mpmc_queue<DataT, size_t N, IndexT=uint32_t, bool lazy_push=false, bool lazy_pop=false> queue;
```
 template parameters:
* *DataT* is the data items type - value_type, should be trivial type
     The total size of the DataT plus the size of the IndexT should be 16 bytes or less,
     as the queue operations use atomic compare and exchange on the _array entries.
* *N* is the compile time size of the queue, if provided as zero 0, the size is set at the call
     to the constructor. The queue size must be power of 2, 1 is valid as 2 to the power of 0.
* *IndexT* - the index type, must be unsigned, with range of four times larger than the size of the queue
* *lazy parameters* - as push and pop operation are successful upon the transaction on the _array[] entry,
     the second operation of increasing the _write_index or _read_index, respectively can be attempted
     immediately (none lazy) or later on (lazy). The best performance should be determined by experimentation.

### Producer

* push(data_type d) -- pushes d into the queue tail, returns true on success, false on full queue.
* push(data_type d, index_type& i) -- pushes d into the queue and sets i to the sequence number of the successful push(), returns true on success, false on full queue.
* push_keep_n(data_type d) -- pushes d into the queue, if the queue is full, it will pop()/discard one data element from the queue and will retry to push it into the queue, returns true after successful placement into the queue.

### Consumer

* pop(data_type& d) -- pop the head of the queue - returns true on success, false on empty queue
* pop(data_type&d, index_type& i) -- pop the head of the queue and updates the index to the entry sequence number, returns true on success false on empty queue
* peek(data_type&d) -- look at the current head of the queue, returns true on sucess, false on empty queue.
* peek(data_type&d, index_type& i) -- to be implemented -- returns the current head of the queue and its index, returns true on successs, false on empty queue.
* pop_if(data_type& d, F& condition) -- evaluate the condition on the current head of the queue, if evaluated to true, try to pop the element, returns true if pop succeed, false if condition is not true or queue is empty
* consume(function) -- consume repeatably the element in the queue, stops when queue is empty or the function returned true to terminate, returns number of elments that were consumed

## Example

Two producers, each producer places N integers into the queue, two consumers, each consumer consumes N numbers from the queue.

```cpp
#include <mpmc_queue.h>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    es::lockfree::mpmc_queue<unsigned> q{32};

    constexpr unsigned N{1000000};
    constexpr unsigned P{2};
    std::atomic<uint64_t> prod_sum{0};
    std::atomic<uint64_t> cons_sum{0};

    auto producer = [&]() {
        for (unsigned x = 0; x < N; ++x) {
            while (!q.push(x))
                ;
            prod_sum += x;
        }
    };
    std::vector<std::thread> producers;
    producers.resize(P);
    for (auto& p : producers) p = std::thread{producer};

    auto consumer = [&]() {
        unsigned v{0};
        for (unsigned x = 0; x < N; ++x) {
            while (!q.pop(v))
                ;
            cons_sum += v;
        }
    };
    std::vector<std::thread> consumers;
    consumers.resize(P);
    for (auto& c : consumers) c = std::thread{consumer};

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();
    std::cout << (cons_sum && cons_sum == prod_sum ? "OK" : "ERROR") << " " << cons_sum << '\n';
    return 0;
}
```

you can try the example using: make build/example2

## Implementation

### Operation

The push/pop operations are defined successful when compare_exchange successed on the internal
buffer's entries. The write / read indexes are changed also using atomic compare_exchange as second step of the operation.

The writer threads and reader threads are cooperative within their group,
so any successful operation on the _array[] entries, can be completed by other threads of the same type.

The push/write and the pop/read indexes are incremented by one as part of successful operation.
The sequence number inside the _array entries hold the value of (index<<1) for empty entry, and ((index<<1)|1)
odd value for entry with data.

* Initialization - clear the _array entries, place the _seq (index<<1) empty/even values indicating the empty entries

* push(value_type d) - as value_type are expected to be small, they are passed by value and not const reference. returns true on successful push(), false if queue is full

* pop(value_type& d) - true if popped an element, false if queue is empty.

* empty() - true if queue is empty, progress the _read_index if needed
* consume() - consumes one or more elements, ends on empty queue or true return value from the fanctor
* peek() - look at the head of the queue, without popping out the element
* pop_if() - pop the head of the queue if functor returns true
* push_keep_n() - push an element, and drop the oldest element from the queue if failed to push and push again. always return true;
* capacity() - returns the capacity of the queue
* size() - returns number of elements in the queue, its value is not guarenteed, as push/pop might be lazy to increment their respective indexes.
* enqueue() / dequeue() - convenience methods call push()/pop() respectively.

### Performance
The bandwidth performance, measured using the src/q_bandwidth.cpp shows that this
queue is faster by two or three times than boost::lockless::queue on some of the runs.

### Implementations Details

1. push(data_type d) operation is composed from the following operations:
  * load _write_index, from the write index get the index into the _array[] by bitwise AND with _index_mask
  * load the _array[] entry, and check the three cases:
  * 1. empty with the same index - then try to fill it in with a compare_and_set operation  changing the _array[] entry from empty to full, including the copy of the data.
  * 2. full with index equal to current index - that means that someone wrote to the _array[] entry and filled it, and we have to co-oparate and increase the write_index.
  * 3. full with previous index, the queue is full, then we return false
  * if we did not succeed with any of the above, we reload the write_index and loop again.
2. pop(value_type& d) - similar to push
 * load read_index from _read_index, and calc the _array[] entry
 * if entry is full with with current read_index, try compare_and_set on the array entry whith new value which is the next empty value (current read_index + queue size)
 * if empty or full with index of equal to current read_index+size, then increament the read_index, as someone clear the entry and place the next empty state (which might have be filled by a writer/producer)
 * if empty with current index - queue is empy and return false
 * if non of the above, re-load read_index and try again.

In short, push/pop - has two compare_and_set operations, first one on the array, and then on the index.
* As the compare_and_set operations can be executed by different threads, there is also an option to instantiate the mpmc_queue<> in lazy mode, where the push/op return after the successful _array[] operation, and leave the indexes to be increamented by the next push/pop operation (in the same thread or other).


### Internal data members

* _write_index
* _read_index
* _index_mask
* _size
* _array[]

### Shared mpmc_queue - fixed size queue between processes
 The es::lockfree:shared_mpmc_queue<> constructor expects a file name in the /dev/shm/ or /dev/hugepages. multiple processes and their threads can access a shared queue. The shared_mpmc_queue provides producer and consumer API.

```cpp
#include<es/lockfree/shared_mpmc_queue.h>

es::lockfree::shared_mpmc_queue<es::lockfree::mpmc_queue<unsigned, 128, unsigned>> shared_q("/dev/shm/mpmc_q000");

// multiple producer processes/threads
auto prod = shared_q.get_producer();
prod.push(123);

 ...
// multiple consumer processes/threads
auto con = shared_q.get_consumer();
unsigned value{};
if (cons.pop(value))
std::Cout <<  ( value == 123 )
```

## Installation
* make -- builds the tests and the performance benchmarks in the build directory
* make cmake -- call cmake to build the same as above
* make report -- will build and run a performance bandwidth report
* make install -- copy the include files to the /use/local/include/es/lockfree/
the package has two header files for
## Testing

to run google test of the queues, run the following

```bash
make gtest_run
```

To generate performance report, including boost queue for comparison:
```bash
make report
```

## Benchmarks

Use the 'make report' to run the q_bandwidth.cpp, it will generate performance report.
the performance report can be processed to generate the summary below per data size.

The benchmark tests the four different favors of the mpmc_queue<> which are differ with regardi
to the incrementing the write and read indexes. 

Here are results from laptop, hyper-threading is on.

```
$ ./scripts/report-processing.pl reports/q-bw-report.20191126-235824.txt
report for data size: 4
fastest 1-to-1:  data_sz: 4 index_sz: 4 queue_name: mpmc_queue<ff> capacity: 64 bandwidth: 49,415,117
fastest 2-to-2:  data_sz: 4 index_sz: 4 queue_name: mpmc_queue<ff> capacity: 64 bandwidth: 12,147,207 (24.58)
fastest 1-to-2:  data_sz: 4 index_sz: 4 queue_name: mpmc_queue<tf> capacity: 64 bandwidth: 14,316,094 (28.97)
fastest 2-to-1:  data_sz: 4 index_sz: 4 queue_name: mpmc_queue<ft> capacity: 64 bandwidth: 14,071,685 (28.48)
boostq  1-to-1:  data_sz: 4 index_sz: - queue_name: boost:lf:queue capacity: 64 bandwidth: 6,074,995 (12.29)
boostq  2-to-2:  data_sz: 4 index_sz: - queue_name: boost:lf:queue capacity: 64 bandwidth: 5,328,217 (10.78)
report for data size: 8
fastest 1-to-1:  data_sz: 8 index_sz: 8 queue_name: mpmc_queue<tf> capacity: 64 bandwidth: 27,853,442
fastest 2-to-2:  data_sz: 8 index_sz: 8 queue_name: mpmc_queue<tf> capacity: 64 bandwidth: 9,294,286 (33.37)
fastest 1-to-2:  data_sz: 8 index_sz: 8 queue_name: mpmc_queue<tf> capacity: 8 bandwidth: 11,121,625 (39.93)
fastest 2-to-1:  data_sz: 8 index_sz: 8 queue_name: mpmc_queue<ft> capacity: 8 bandwidth: 13,297,038 (47.74)
boostq  1-to-1:  data_sz: 8 index_sz: - queue_name: boost:lf:queue capacity: 2 bandwidth: 5,802,189 (20.83)
boostq  2-to-2:  data_sz: 8 index_sz: - queue_name: boost:lf:queue capacity: 64 bandwidth: 5,179,598 (18.60)
report for data size: 12
fastest 1-to-1:  data_sz: 12 index_sz: 4 queue_name: mpmc_queue<tf> capacity: 64 bandwidth: 28,228,040
fastest 2-to-2:  data_sz: 12 index_sz: 4 queue_name: mpmc_queue<tf> capacity: 8 bandwidth: 9,419,344 (33.37)
fastest 1-to-2:  data_sz: 12 index_sz: 4 queue_name: mpmc_queue<tf> capacity: 64 bandwidth: 11,157,596 (39.53)
fastest 2-to-1:  data_sz: 12 index_sz: 4 queue_name: mpmc_queue<ft> capacity: 8 bandwidth: 13,770,559 (48.78)
boostq  1-to-1:  data_sz: 12 index_sz: - queue_name: boost:lf:queue capacity: 64 bandwidth: 6,110,304 (21.65)
boostq  2-to-2:  data_sz: 12 index_sz: - queue_name: boost:lf:queue capacity: 64 bandwidth: 5,336,504 (18.90)

```

## Future Optimizations

* relax the strong ordering of the atomic operations

* In the push/pop - attempt to complete the operations using transactional memory operations (xbegin/xend) and only on failure fall back to atomic compare_exchange operations.

## Next steps

* performance report summary
* Collecting performance reports from different machines, platforms.
* You can run, test and send me performance report files

## Feedback

Please send your feedback to Erez Strauss <erez@erezstrauss.com> with subject line MPMCQ ...

## Acknowledgement

* I would like to thanks Alon Cohen for the ideas for part of the implemented functionality and endless discussion about this mpmc_queue.

## External References

Thanks also goes to the following queues implementations,

Dmitry Vyukov MPMC Queue
* http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue \
 This queue, uses compare_exchange on the read/write indexes and sequence numbers within the buffer entries.

Boost MPMC lockfree
*   https://www.boost.org/doc/libs/1_71_0/doc/html/boost/lockfree/queue.html


## LICENSE

 MIT, see LICENSE file.
