#include <jinx/assert.hpp>
#include <iostream>

#include <jinx/async.hpp>
#include <jinx/queue.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

int main(int argc, const char* argv[])
{
    {
        detail::QueueBase<std::array<int, 3>> queue{};
        jinx_assert(queue.empty());

        queue.emplace(1);
        jinx_assert(not queue.empty());
        jinx_assert(not queue.full());
        jinx_assert(queue.size() == 1);
        jinx_assert(queue.front() == 1);

        queue.emplace(2);
        queue.emplace(3);
        jinx_assert(queue.full());
        jinx_assert(queue.size() == 3);
        jinx_assert(queue.front() == 1);

        queue.pop();
        jinx_assert(queue.front() == 2);

        queue.pop();
        queue.pop();
        jinx_assert(queue.empty());
    }

    {
        detail::QueueBase<std::array<int, 3>> queue{};
        jinx_assert(queue.empty());

        queue.emplace(1);
        queue.emplace(2);
        queue.emplace(3);

        queue.pop();

        queue.emplace(4);

        jinx_assert(queue.front() == 2);
        queue.pop();
        jinx_assert(queue.front() == 3);
        queue.pop();
        jinx_assert(queue.front() == 4);
        queue.pop();

        jinx_assert(queue.empty());
    }
    return 0;
}
