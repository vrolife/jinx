#include <jinx/assert.hpp>
#include <string>
#include <iostream>

#define JINX_RESULT_THROW 1

#include <jinx/macros.hpp>

#undef JINX_NO_DISCARD
#define JINX_NO_DISCARD

#include <jinx/result.hpp>
#include <jinx/resultstrict.hpp>
#include <jinx/overload.hpp>

using namespace jinx;

struct LazyHandler {
    template<typename T>
    void operator()(T&& val) { }
};

JINX_NO_DISCARD
ResultBool foo() {
    return true;
}

int main(int argc, const char* argv[]) // NOLINT
{
    try {
        {
            ResultBool xxx(false);
        }
        jinx_assert(false);
    } catch(std::logic_error& e) {
    }

    try {
        ResultBool xxx(false);
        xxx >> JINX_IGNORE_RESULT;
    } catch(std::logic_error& e) {
        jinx_assert(false);
    }

    try {
        {
            ResultStrict<int, void*> xxx{1};
        }
        jinx_assert(false);
    } catch(const std::logic_error& e) {
    }

    try {
        ResultStrict<int, std::string> xxx{1};
        xxx(
            [](int) { },
            [](const std::string&) { }
        ) >> JINX_IGNORE_RESULT;
    } catch(const std::logic_error& e) {
        jinx_assert(false);
    }

    try {
        ResultStrict<int, std::string> xxx{1};
        xxx(overload(
            [](int) { },
            [](const std::string&) { }
        )) >> JINX_IGNORE_RESULT;
    } catch(std::logic_error& e) {
        jinx_assert(false);
    }

    try {
        bool flag{false};
        ResultStrict<int, std::string> xxx{1};
        xxx(
            [&](int) { flag = true; },
            [](const std::string&) { }
        ) >> JINX_IGNORE_RESULT;
        jinx_assert(flag);
    } catch(std::logic_error& e) {
        jinx_assert(false);
    }

    try {
        bool flag{false};
        ResultStrict<int, std::string> xxx{1};
        xxx(
            [](int) { },
            [&](const std::string&) { flag = true; },
            [&]() { flag = true; }
        ) >> JINX_IGNORE_RESULT;
        jinx_assert(!flag);
    } catch(std::logic_error& e) {
        jinx_assert(false);
    }

    // check lazy
    try {
        LazyHandler lazy{};
        ResultStrict<int, std::string> xxx{1};
        xxx(lazy);
        jinx_assert(false);
    } catch(...) {
    }

    try {
        foo();
    } catch (...) { }

    return 0;
}
