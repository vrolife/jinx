#include <jinx/assert.hpp>
#include <cstring>
#include <unistd.h>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/posix.hpp>

using namespace jinx;

typedef AsyncEngine<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

static int counter = 0;

class Cancel : public AsyncRoutine {
    int _w{-1};
    asyncio::Write _async_write{};
    std::function<void(void)> _fun;

public:
    Cancel() = default;

    Cancel& operator ()(int wfd, std::function<void(void)>&& fun)
    {
        async_start(&Cancel::prepare);
        _w = wfd;
        _fun = std::move(fun);
        return *this;
    }

    Async prepare() {
        return async_yield(&Cancel::check_and_cancel);
    }

    Async check_and_cancel() {
        jinx_assert(counter == 1);
        counter += 1;
        _fun();
        return async_return();
    }
};

class Reader : public AsyncRoutine {
    int _rfd{-1};
    asyncio::Read _async_read{};
    char _buf[6]{};

public:
    Reader() = default;

    Reader& operator ()(int rfd) {
        async_start(&Reader::read);
        _rfd = rfd;
        return *this;
    };

    Async read() {
        counter += 1;
        return async_await(_async_read(_rfd, SliceMutable{_buf, 6}), &Reader::finish);
    }
// LCOV_EXCL_START
// never reach
    Async finish() {
        jinx_assert(memcmp("hello", &_buf[0], 5) == 0);
        return async_return();
    }
// LCOV_EXCL_STOP
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    int fds[2];
    jinx_assert(pipe(fds) != -1);

    asyncio::IOHandleType fds0{fds[0]};
    asyncio::IOHandleType fds1{fds[1]};

    fds1.set_non_blocking(true);
    fds0.set_non_blocking(true);

    auto task = loop.task_new<Reader>(fds[0]);
    loop.task_new<Cancel>(fds[1], [&](){
        loop.cancel(task) >> JINX_IGNORE_RESULT;
    });

    loop.run();
    
    jinx_assert(counter == 2);
    return 0;
}
