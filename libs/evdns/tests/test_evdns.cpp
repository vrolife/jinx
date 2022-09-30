
#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/evdns/evdns.hpp>

using namespace jinx;
using namespace jinx::evdns;

class AsyncTest : public AsyncRoutine {
    typedef AsyncRoutine BaseType;

    evdns_base* _base{};
    evdns::AsyncResolve _resolve{};

public:
    AsyncTest& operator ()(evdns_base* base) {
        _base = base;
        async_start(&AsyncTest::foo);
        return *this;
    }

    Async foo() {
        return *this / _resolve(_base, "localhost", DNSType::A) / &AsyncTest::bar;
    }

    Async bar() {
        auto* addr = _resolve.get_address();
        jinx_assert(addr[0] == 127);
        jinx_assert(addr[1] == 0);
        jinx_assert(addr[2] == 0);
        jinx_assert(addr[3] == 1);
        evdns_base_free(_base, 0);
        return this->async_return();
    }
};

int main(int argc, char *argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    evdns_base* base = evdns_base_new(eve.get_event_base(), 0);
    evdns_base_load_hosts(base, "/etc/hosts");
    evdns_base_nameserver_ip_add(base, "8.8.8.8");

    loop.task_new<AsyncTest>(base);

    loop.run();
    return 0;
}
