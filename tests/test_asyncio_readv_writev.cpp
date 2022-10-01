#include <jinx/assert.hpp>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

class AsyncTest : public AsyncRoutine {
    int _fds[2];

    SliceConst wbuf[2]{};

    SliceMutable rbuf[2]{};
    char rbuf0[5]{};
    char rbuf1[5]{};

    asyncio::WriteV _writev{};
    asyncio::ReadV _readv{};

public:
    AsyncTest& operator ()() {
        jinx_assert(::pipe(_fds) != -1);

        asyncio::IOHandleType fdr{_fds[0]};
        asyncio::IOHandleType fdw{_fds[1]};
        fdr.set_non_blocking(true);
        fdw.set_non_blocking(true);
        fdr.release();
        fdw.release();

        wbuf[0]._data = "hello";
        wbuf[0]._size = 5;
        wbuf[1]._data = "world";
        wbuf[1]._size = 5;

        async_start(&AsyncTest::write);
        return *this;
    }

    Async write() {
        return *this
            / _writev(_fds[1], wbuf, 2)
            / &AsyncTest::read;
    }

    Async read() {
        rbuf[0]._data = rbuf0;
        rbuf[0]._size = 5;
        rbuf[1]._data = rbuf1;
        rbuf[1]._size = 5;
        return *this
            / _readv(_fds[0], rbuf, 2)
            / &AsyncTest::check;
    }

    Async check() {
        jinx_assert(std::memcmp(rbuf0, "hello", 5) == 0);
        jinx_assert(std::memcmp(rbuf1, "world", 5) == 0);
        return this->async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    loop.task_new<AsyncTest>();

    loop.run();
    return 0;
}
