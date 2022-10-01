#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <jinx/assert.hpp>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/logging.hpp>

#define PAGE_SIZE 4096

using namespace jinx;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

int counter = 0;

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
        jinx_assert(counter == 0);
        counter += 1;
        char buf[PAGE_SIZE];
        return async_await(_async_write(_wfd, SliceConst{buf, PAGE_SIZE}), &Writer::write_hello);
    }

    Async write_hello() {
        jinx_assert(counter == 1);
        counter += 1;
        return async_await(_async_write(_wfd, SliceConst{"hello", 5}), &Writer::finish);
    }

    Async finish() {
        return async_return();
    }
};

class Reader : public AsyncRoutine {
    int _rfd{-1};
    asyncio::Read _async_read{};
    char _buf[PAGE_SIZE]{};

public:
    Reader() = default;

    Reader& operator ()(int rfd) {
        async_start(&Reader::read);
        _rfd = rfd;
        return *this;
    };

    Async read() {
        jinx_assert(counter == 2);
        counter += 1;
        return async_await(_async_read(_rfd, SliceMutable{_buf, PAGE_SIZE}), &Reader::read_hello);
    }

    Async read_hello() {
        jinx_assert(counter == 3);
        counter += 1;
        return async_await(_async_read(_rfd, SliceMutable{_buf, 5}), &Reader::finish);
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
    jinx_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    asyncio::IOHandleType fds0{fds[0]};
    asyncio::IOHandleType fds1{fds[1]};

    fds0.set_non_blocking(true);
    fds1.set_non_blocking(true);

    int size = PAGE_SIZE;
    jinx_assert(setsockopt(fds[1], SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == 0);

    loop.task_new<Reader>(fds[0]);
    loop.task_new<Writer>(fds[1]);

    loop.run();
    return 0;
}
