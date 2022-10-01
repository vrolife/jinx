#include <jinx/assert.hpp>
#include <cstring>
#include <unistd.h>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

class Writer : public AsyncRoutine {
    int _wfd{-1};
    asyncio::Write _async_write{};

public:
    Writer() = default;

    Writer& operator ()(int wfd)
    {
        async_start(&Writer::write);
        _wfd = wfd;
        return *this;
    }

    Async write() {
        return async_await(_async_write(_wfd, SliceConst{"hello", 5}), &Writer::async_return);
    }

    Async finish() {
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
        return async_await(_async_read(_rfd, SliceMutable{_buf, 6}), &Reader::finish);
    }

    Async finish() {
        jinx_assert(memcmp("hello", &_buf[0], 5) == 0);
        return async_return();
    }
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

    loop.task_new<Writer>(fds[1]);
    loop.task_new<Reader>(fds[0]);

    loop.run();
    return 0;
}
