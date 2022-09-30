#include <jinx/assert.hpp>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

#include <iostream>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncEngine<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

class AsyncTest : public AsyncRoutine {
    int _fds[2];

    struct msghdr whdr{};
    struct iovec wbuf[2]{};

    struct msghdr rhdr{};
    struct iovec rbuf[2]{};
    char rbuf0[5]{};
    char rbuf1[5]{};

    asyncio::SendMsg _sendmsg{};
    asyncio::RecvMsg _recvmsg{};

public:
    AsyncTest& operator ()() {
        auto res = ::socketpair(AF_UNIX, SOCK_STREAM, 0, _fds);
        if (res == -1) {
            std::cout << strerror(errno) << std::endl;
            abort();
        }

        asyncio::IOHandleType fdr{_fds[0]};
        asyncio::IOHandleType fdw{_fds[1]};
        fdr.set_non_blocking(true);
        fdw.set_non_blocking(true);
        fdr.release();
        fdw.release();
        
        async_start(&AsyncTest::write);
        return *this;
    }

    Async write() {
        wbuf[0].iov_base = const_cast<char*>("hello");
        wbuf[0].iov_len = 5;
        wbuf[1].iov_base = const_cast<char*>("world");
        wbuf[1].iov_len = 5;
        whdr.msg_name = nullptr;
        whdr.msg_namelen = 0;
        whdr.msg_iov = wbuf;
        whdr.msg_iovlen = 2;
        whdr.msg_control = nullptr;
        whdr.msg_controllen = 0;
        whdr.msg_flags = 0;
        return *this
            / _sendmsg(_fds[1], &whdr, 0)
            / &AsyncTest::read;
    }

    Async read() {
        rbuf[0].iov_base = rbuf0;
        rbuf[0].iov_len = 5;
        rbuf[1].iov_base = rbuf1;
        rbuf[1].iov_len = 5;
        rhdr.msg_name = nullptr;
        rhdr.msg_namelen = 0;
        rhdr.msg_iov = rbuf;
        rhdr.msg_iovlen = 2;
        rhdr.msg_control = nullptr;
        rhdr.msg_controllen = 0;
        rhdr.msg_flags = 0;
        return *this
            / _recvmsg(_fds[0], &rhdr, 0)
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
