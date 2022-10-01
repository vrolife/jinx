#include <jinx/async.hpp>
#include <jinx/buffer.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;
using namespace jinx::buffer;

class AsyncTest : public AsyncRoutine {
    typedef AsyncRoutine BaseType;
public:
    AsyncTest& operator ()() {
        async_start(&AsyncTest::exit);
        return *this;
    }

    Async exit() {
        return async_return();
    }
};

#define SHORT_TEMPLATE_TYPE(name, base) struct name : base { };

using AllocatorType = buffer::BufferAllocator
<
    posix::MemoryProvider, 
    TaskBufferredConfig<AsyncTest>
>;

int main(int argc, char const *argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};

    assert(loop.task_allocate<TaskBufferredConfig<AsyncTest>>(&allocator).is(Successful_));
    
    loop.run();
    return 0;
}
