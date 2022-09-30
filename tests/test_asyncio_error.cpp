#include <unistd.h>

#include <jinx/assert.hpp>
#include <cstring>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/posix.hpp>

using namespace jinx;
typedef AsyncEngine<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

static int counter = 0;

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

protected:
    Async handle_error(const error::Error& error) override {
        counter += 1;

        if (error.category() != posix::category_posix()) {
            abort();
        }

        if (static_cast<std::errc>(error.value()) != std::errc::bad_file_descriptor) {
            abort();
        }
        return async_return();
    }

    Async read() {
        counter += 1;
        return *this / _async_read(_rfd, SliceRead{_buf, 6}) / &Reader::finish;
    }

    Async finish() { // NOLINT
        counter += 1;
        abort();;
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    int filedesc = 0x28749;
    ::close(filedesc);

    loop.task_new<Reader>(filedesc);

    loop.run();
    jinx_assert(counter == 2);
    return 0;
}
