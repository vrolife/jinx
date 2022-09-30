#include <unistd.h>
#include <fcntl.h> 

#include <sys/socket.h>

#include <cstring>
#include <jinx/assert.hpp>
#include <array>
#include <iostream>

#include <jinx/buffer.hpp>
#include <jinx/libevent.hpp>
#include <jinx/posix.hpp>

using namespace jinx;
using namespace jinx::buffer;

struct BufferConfig
{
    constexpr static char const* Name = "Example";
    static constexpr const size_t Size = 1500;
    static constexpr const size_t Reserve = 100;
    static constexpr const long Limit = -1;
    
    struct Information { };
};

typedef BufferAllocator<posix::MemoryProvider, BufferConfig> AllocatorType;
typedef typename AllocatorType::BufferType BufferType;
typedef typename AllocatorType::Allocate Allocate;

class AsyncTest1 : public AsyncRoutine {
    AllocatorType* _allocator{};
    BufferType _buffer{};

public:
    AsyncTest1& operator ()(AllocatorType* allocator) {
        _allocator = allocator;
        async_start(&AsyncTest1::allocate);
        return *this;
    }

    Async allocate() {
        _buffer = _allocator->allocate(BufferConfig{});
        return this->async_yield(&AsyncTest1::release);
    }

    Async release() {
        _buffer.reset();
        return this->async_return();
    }
};

class AsyncTest2 : public AsyncRoutine {
    AllocatorType* _allocator{};
    Allocate _allocate{};

public:
    AsyncTest2& operator ()(AllocatorType* allocator) {
        _allocator = allocator;
        async_start(&AsyncTest2::prepare);
        return *this;
    }

    Async prepare() {
        return this->async_yield(&AsyncTest2::allocate);
    }

    Async allocate() {
        return *this / _allocate(_allocator, BufferConfig{}) / &AsyncTest2::finish;
    }

    Async finish() {
        return this->async_return();
    }
};

int main(int argc, const char* argv[])
{
    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};

    allocator.reconfigure<BufferConfig>(4, 5);

    jinx_assert(allocator.active_buffer_count() == 4);

    auto buf1 = allocator.allocate(BufferConfig{});
    jinx_assert(allocator.active_buffer_count() == 4);
    jinx_assert(allocator.reserve_buffer_count() == 3);

    auto buf2 = allocator.allocate(BufferConfig{});
    auto buf3 = allocator.allocate(BufferConfig{});
    auto buf4 = allocator.allocate(BufferConfig{});
    jinx_assert(allocator.active_buffer_count() == 4);
    jinx_assert(allocator.reserve_buffer_count() == 0);

    auto buf5 = allocator.allocate(BufferConfig{});
    jinx_assert(allocator.active_buffer_count() == 5);
    jinx_assert(allocator.reserve_buffer_count() == 0);

    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    loop.task_new<AsyncTest1>(&allocator);
    loop.task_new<AsyncTest2>(&allocator);

    loop.run();
}
