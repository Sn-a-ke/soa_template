
# soa container template
todo

## Features
* todo
* todo

## Usage
`SoA.cpp`

```CPP
#include <iostream>
#include <array>
#include <vector>
#include <tuple>

#include "soa.h"

int main()
{
	//some custom type
	struct Vec3 : public std::array<float, 3>
	{
		Vec3(float x, float y, float z) : std::array<float, 3>({ x,y,z }) {}
		Vec3(float In) : Vec3(In, In, In) {}
	};

	using SoAType = soa<std::vector<int>, int, float, Vec3>;
	SoAType Soa;

	int Count = 128;
	Soa.reserve(Count);


	// add elements to SoA
	for (int i = 0; i < Count; i++)
	{
		std::vector<int> el1;
		el1.resize(3, i);
		int el2 = i;
		float el3 = static_cast<float>(i);
		Vec3 el4 = Vec3(static_cast<float>(i));

		Soa.push_back(el1, el2, el3, el4); // add new element to end
	}

	Soa.remove_at(10);
	Soa.insert(10, { 1,2, 3 }, 123, 123, Vec3(123));
	Soa.push_back({ 1,2, 3 }, 123, 123, Vec3(123));

	// swap indexes
	Soa.swap(1, 2);

	{
		auto [a1, b1, c1, d1] = Soa.get_tuple(1);
		auto [a2, b2, c2, d2] = Soa.get_tuple(2);
		printf("%d, %d, %f, %f \n", int(a1.size()), b1, c1, d1[0]);
		printf("%d, %d, %f, %f \n", int(a1.size()), b1, c1, d2[0]);
	}

	// sort by single type lambda
	Soa.sort([](const std::vector<int>& A, const std::vector<int>& B) -> bool { return A.size() < B.size(); });
	Soa.sort([](const float& A, const float& B) -> bool { return A < B; });


	// for_each by type
	Soa.for_each([](float& A, const int& B) { A += B; });

	// iterator
	for (auto It = Soa.iterator(); It; ++It)
	{
		std::tuple<std::vector<int>&, int&, float&, Vec3&> TGet = *It;
		auto [el1, el2, el3, el4] = TGet;
	}

	// c++17 tuple iterator
	for (auto [el1, el2, el3, el4] : Soa)
	{
		printf("%d, %d, %f, %f\n", int(el1.size()), el2, el3, el4[0]);
	}

	// c++17 tuple iterator
	for (auto [el1, el2, el3, el4] : Soa.range_iterator())
	{
		el1 = { 5, 3, 2, 0 };
		el2 = 123;
		el3 = 123.0;
		el4 = Vec3(1);
	}

	// optional single type range iterator
	for (std::vector<int>& Vector : Soa.range_iterator<std::vector<int>>())
	{
		for (auto& i : Vector)
		{
			i++;
		}
	}

	// optional type range iterator  c++17
	for (auto [el1, el2] : Soa.range_iterator<int, float>())
	{
		el1 = 321;
		el2 = 321.f;
	}

	// incremental iterator
	for (auto It = Soa.iterator(); It; ++It)
	{
		std::tuple<std::vector<int>&, int&, float&, Vec3&> Tup = *It;
	}

	// optional const type iterator
	for (auto It = Soa.iterator_const<float, int>(); It; ++It)
	{
		std::tuple<const float&, const int&> Tup = *It;
	}

	// optional type iterator
	for (auto It = Soa.iterator<Vec3, const std::vector<int>>(); It; ++It)
	{
		if (std::get<1>(*It).size() == 0)
		{
			It.remove_current();
			continue;
		}
		std::tuple<Vec3&, const std::vector<int>&> Tup = *It;
		std::get<0>(Tup) = Vec3(1);
	}

	// for () get by index
	for (int32_t i = 0; i < Soa.size(); ++i)
	{
		float& Float = Soa.get<float>(i);
		int& Int = Soa.get<int>(i);
		Vec3& Vec = Soa.get<Vec3>(i);
		std::vector<int>& Vector = Soa.get<std::vector<int>>(i);
	}
}
```

## Benchmarks
```
todo
```