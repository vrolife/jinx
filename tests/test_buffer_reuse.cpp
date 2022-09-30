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

int main(int argc, const char* argv[])
{
    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};

    allocator.reconfigure<BufferConfig>(4, 5);

    jinx_assert(allocator.active_buffer_count() == 4);

    auto buf = allocator.allocate(BufferConfig{});
    auto* begin = buf->begin();
    auto* end = buf->end();

    jinx_assert(buf->size() == 0);
    jinx_assert(buf->capacity() == 1500);

    buf->commit(100) >> JINX_IGNORE_RESULT;
    jinx_assert(buf->size() == 100);
    jinx_assert(buf->capacity() == 1400);
    
    jinx_assert(begin == buf->begin());
    jinx_assert((end + 100) == buf->end());

    buf->consume(50) >> JINX_IGNORE_RESULT;

    jinx_assert(buf->size() == 50);
    jinx_assert(buf->capacity() == 1400);

    auto* begin2 = buf->begin() - 50;
    jinx_assert((begin == begin2));

    buf->cut(50) >> JINX_IGNORE_RESULT;
    jinx_assert(buf->size() == 50);
    jinx_assert(buf->capacity() == 1400);
    jinx_assert(buf->memory_size() == 1450);

    buf = {};
    buf = allocator.allocate(BufferConfig{});
    jinx_assert(begin == buf->begin());

    jinx_assert(buf->size() == 0);
    jinx_assert(buf->capacity() == 1500);
    jinx_assert(buf->memory_size() == 1500);
}
