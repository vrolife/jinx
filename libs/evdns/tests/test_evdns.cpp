
#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/evdns/evdns.hpp>

using namespace jinx;
using namespace jinx::evdns;

class AsyncTest : public AsyncRoutine {
    typedef AsyncRoutine BaseType;

    struct evdns_base* _base{};
    struct evdns_server_port* _server_port{};
    evdns::AsyncResolve _resolve{};

public:
    AsyncTest& operator ()(struct evdns_base* base, struct evdns_server_port* server_port) {
        _base = base;
        _server_port = server_port;
        async_start(&AsyncTest::foo);
        return *this;
    }

    Async foo() {
        return *this / _resolve(_base, "whoyouare", DNSType::A) / &AsyncTest::bar;
    }

    Async bar() {
        auto* addr = _resolve.get_address();
        jinx_assert(addr[0] == 1);
        jinx_assert(addr[1] == 2);
        jinx_assert(addr[2] == 3);
        jinx_assert(addr[3] == 4);
        evdns_base_free(_base, 0);
        evdns_close_server_port(_server_port);
        return this->async_return();
    }
};

void on_dns_request(struct evdns_server_request *request, void *data)
{
    unsigned int addr = inet_addr("1.2.3.4");
    evdns_server_request_add_a_reply(request, "whoyouare", 1, &addr, 60);
    evdns_server_request_respond(request, 0);
}

int main(int argc, char *argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    evdns_base* base = evdns_base_new(eve.get_event_base(), 0);
    // evdns_base_load_hosts(base, "/etc/hosts");
    
    int server_socket = ::socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = 0;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    jinx_assert(::bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) != -1);

    socklen_t size = sizeof(server_addr);
    getsockname(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), &size);
    jinx_assert(size == sizeof(server_addr));

    evutil_make_socket_nonblocking(server_socket);

    auto* server_port = evdns_add_server_port_with_base(eve.get_event_base(), server_socket, 0, on_dns_request, nullptr);

    evdns_base_nameserver_sockaddr_add(base, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr), 0);

    loop.task_new<AsyncTest>(base, server_port);

    loop.run();
    return 0;
}
