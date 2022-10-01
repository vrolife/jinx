#ifndef __server_hpp__
#define __server_hpp__

#include <jinx/async.hpp>
#include <jinx/logging.hpp>
#include <jinx/buffer.hpp>
#include <jinx/libevent.hpp>
#include <jinx/streamsocket.hpp>
#include <jinx/http/webapp.hpp>

#include "status.hpp"
#include "static.hpp"

#include "appdata.hpp"

namespace example {

using namespace jinx;
using namespace jinx::http;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

struct Root {
    typedef PageIndex Index;
    struct Status {
        constexpr static const char* Name = "status.json";
        typedef PageStatus Index;
        typedef std::tuple<> ChildNodes;
    };
    struct IndexHtml {
        constexpr static const char* Name = "index.html";
        typedef PageIndex Index;
        typedef std::tuple<> ChildNodes;
    };
    struct AppJs {
        constexpr static const char* Name = "app.js";
        typedef FileAppJs Index;
        typedef std::tuple<> ChildNodes;
    };
    typedef std::tuple<IndexHtml, Status, AppJs> ChildNodes;
};

typedef WebConfig<Root> ExampleAppConfig;

class HandshakeSocket;

typedef buffer::BufferAllocator
<
    posix::MemoryProvider, 
    ExampleAppConfig::HTTPConfig::BufferConfig, 
    ExampleAppConfig::BufferConfig,
    buffer::TaskBufferredConfig<HandshakeSocket>
> AllocatorType;

} // namespace example

#endif
