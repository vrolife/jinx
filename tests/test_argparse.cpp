#include <unistd.h>
#include <fcntl.h> 

#include <sys/socket.h>

#include <cstring>
#include <jinx/assert.hpp>
#include <array>
#include <iostream>
#include <sstream>

#include <jinx/async.hpp>
#include <jinx/argparse.hpp>

using namespace jinx::argparse;
using namespace jinx;

const std::array<Argument, 6> arguments
{{
    { "peer-network", "", 0, 100, "peer-network" },
    { "password", "", 0, 100, "password", true },
    { "int", int(0), 0, 100, "peer-network" },
    { "size", size_t(0), size_t(0), size_t(100), "password", true },
    { "float", float(0), float(0), float(100), "password", true },
    { "bool", bool(false), "password", true },
}};

int main(int argc, const char* argv[])
{
    RecordCategory category_config{};

    std::array<const char*, 8> args{{
        "test",
        "--peer-network=1.2.3.4",
        "--password=123",
        "--int=99",
        "--size=99",
        "--float=1.23",
        "--bool=true",
        nullptr
    }};

    parse_argv(args.size() - 1, args.data(), category_config, arguments.data(), arguments.size());

    RecordAbstract* ptr = category_config.check<RecordSecret<std::string>>("password");

    RecordVisitorGetValue<const std::string&> val{};
    jinx_assert(ptr->visit(val) == false);
    jinx_assert(val._value != "123");

    return 0;
}
