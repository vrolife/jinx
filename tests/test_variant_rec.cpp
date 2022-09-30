#include <string>

#include <jinx/variant.hpp>

#ifndef VARIANT
#define VARIANT jinx::VariantRecursive
#endif

int main(int argc, const char* argv[])
{
    VARIANT<int, std::string> v{1};

    jinx_assert(!v.as_pointer<long>().unwrap());
    
    jinx_assert(!v.as_pointer<std::string>().unwrap());
    
    VARIANT<int, std::string> x{};
    x = std::move(v);

    jinx_assert(!v.as_pointer<int>().unwrap());

    jinx_assert(x.as_pointer<int>().unwrap());

    return 0;
}
