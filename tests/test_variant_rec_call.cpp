#include <jinx/assert.hpp>
#include <string>

#include <jinx/variant.hpp>
#include <jinx/overload.hpp>

#ifndef VARIANT
#define VARIANT jinx::VariantRecursive
#endif

int main(int argc, const char* argv[])
{
    int counter = 0;

    VARIANT<int, std::string> val{1};

    auto fun = jinx::overload(
        [&](int) {
            counter += 1;
        },
        [&](const std::string&) {
            counter += 10;
        }
    );

    val(fun) >> JINX_IGNORE_RESULT;

    val.emplace<std::string>("sdf");

    val(fun) >> JINX_IGNORE_RESULT;

    jinx_assert(counter == 11);

    return 0;
}
