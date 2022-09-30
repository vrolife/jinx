#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <utility>

#define __JINX_DEBUG_VARIANT_VOLATILE_TYPE 1

#include <jinx/variant.hpp>
#include <jinx/overload.hpp>

volatile size_t counter = 0;
volatile bool flag = 0;

template<size_t I>
struct Foo {
	bool operator ==(const Foo&) volatile {
		counter ++;
		return false;
	}
};

template<size_t... I>
auto make_lut(std::index_sequence<I...>)
{
	return jinx::VariantLookupTable<Foo<I>...>{};
}

template<size_t... I>
auto make_rec(std::index_sequence<I...>)
{
	return jinx::VariantRecursive<Foo<I>...>{};
}

template<typename V, size_t N>
void test(size_t max) 
{
	V val{Foo<N>{}};
	V val2{Foo<N>{}};

	auto t0 = std::chrono::high_resolution_clock::now();
	while (max --) {
		flag = (val == val2);
	}
	auto t1 = std::chrono::high_resolution_clock::now();

	auto dur = t1 - t0;
	auto t = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
	std::cout << t << std::endl;
}

template<size_t N>
void test_N() {
	typedef decltype(make_lut(std::make_index_sequence<N>())) LUT;
	typedef decltype(make_rec(std::make_index_sequence<N>())) REC;

	const size_t max = 1000UL * 1000UL * 1000UL * 1;
	
	std::cout << "LUT 0 -> ";
	test<LUT, 0>(max);

	std::cout << "REC 0 -> ";
	test<REC, 0>(max);

	std::cout << "LUT N - 1 -> ";
	test<LUT, N - 1>(max);

	std::cout << "REC N - 1 -> ";
	test<REC, N - 1>(max);
}

int main(int argc, const char* argv[])
{
	std::cout << "WARM " << std::endl;
	test_N<5>();

	std::cout << "T 5 " << std::endl;
	test_N<5>();

	std::cout << "T 10 " << std::endl;
	test_N<10>();

	std::cout << "T 20 " << std::endl;
	test_N<20>();

	std::cout << "CNT " << counter << std::endl;
    return 0;
}

