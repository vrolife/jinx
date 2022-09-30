#include <iostream>
#include <string>
#include <vector>

#include <jinx/variant.hpp>
#include <jinx/overload.hpp>

#ifndef VARIANT
#define VARIANT jinx::VariantRecursive
#endif

int main(int argc, const char* argv[])
{
    VARIANT<std::vector<char>, std::string> val{std::string{""}};
    val.as_pointer<std::string>() >> JINX_IGNORE_RESULT;
    VARIANT<std::vector<char>, std::string> val2 = val;
    val2(jinx::overload(
        [](const std::string& s) {
            std::cout << s << std::endl;
        },
        [](const std::vector<char>&) { }
    )) >> JINX_IGNORE_RESULT;
    return 0;
}
