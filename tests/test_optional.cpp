#include <jinx/assert.hpp>
#include <string>

#include <jinx/optional.hpp>

using namespace jinx;

template<typename T, typename Optional<T>::ValueType init>
void test_state() 
{
    Optional<T> a;
    Optional<T> b{init};
    Optional<T> c{init};

    c.reset();

    jinx_assert(a.empty());
    jinx_assert(not b.empty());
    jinx_assert(c.empty());

    c.emplace(init);
    jinx_assert(not c.empty());

    Optional<T> d{std::move(c)};
    jinx_assert(c.empty()); // NOLINT
    jinx_assert(not d.empty());

    Optional<T> e{d};
    jinx_assert(not d.empty());
    jinx_assert(not e.empty());

    d.reset();
    jinx_assert(d.empty());
    d = e;

    jinx_assert(not d.empty());
    jinx_assert(not e.empty());

    Optional<T> f(e); // NOLINT

    jinx_assert(not e.empty());
    jinx_assert(not f.empty());
}

void test_value()
{
    Optional<std::string> a{"hello"};
    Optional<std::string> b{a};

    jinx_assert(a.get() == "hello");
    jinx_assert(b.get() == "hello");

    Optional<std::string> c{std::move(a)};
    jinx_assert(a.get().size() == 0);  // NOLINT
    jinx_assert(c.get() == "hello");

    std::string x = "xx";
    Optional<std::string> d{"hello"};
    d.emplace(std::move(x));
    jinx_assert(d.get() == "xx");

    d = std::move(b);
    jinx_assert(d.get() == "hello");
    jinx_assert(b.get().size() == 0);  // NOLINT

    Optional<std::string> e;
    e = d;
    jinx_assert(d.get() == "hello");
    jinx_assert(e.get() == "hello");
}

int main(int argc, const char* argv[])
{
    test_state<void, nullptr>();
    test_state<int, 0>();
    test_value();
    return 0;
}
