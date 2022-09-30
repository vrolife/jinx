#include <fcntl.h>

#include <jinx/assert.hpp>

#include <jinx/async.hpp>
#include <jinx/posix.hpp>

using namespace jinx;

int main(int argc, const char* argv[])
{
    posix::Socket sock(::socket(AF_INET, SOCK_DGRAM, 0));
    sock.set_non_blocking(true);

    jinx_assert((fcntl(sock.native_handle(), F_GETFL) & O_NONBLOCK) != 0);

    sock.set_non_blocking(false);
    jinx_assert((fcntl(sock.native_handle(), F_GETFL) & O_NONBLOCK) == 0);

    jinx_assert(sock.bind("127.0.0.1", 8327).unwrap() == 0);
    return 0;
}
