#include <jinx/overload.hpp>

using namespace jinx;

int main(int argc, const char* argv[])
{
    auto fun1 = jinx::overload(
        [](){}
    );

    fun1();

    auto fun2 = jinx::overload(
        [](){},
        [](int){}
    );

    fun2();
    fun2(1);

    return 0;
}
