#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <string>
#include <list>

#include "BinaryLove3.hpp"

struct test0
{
	uint32_t v0 = 3;
	uint32_t v1 = 2;
	float_t v2 = 2.5f;
	char v3 = 'c';
	struct
	{
		std::vector<int> vec_of_trivial = { 1, 2, 3 };
		std::vector<std::string> vec_of_nontrivial = { "I am a Fox!", "In a big Box!" };
		std::string str = "Foxes can fly!";
		std::list<int> non_random_access_container = { 3, 4, 5 };
	} non_trivial;
	struct
	{
		uint32_t v0 = 1;
		uint32_t v1 = 2;
	} trivial;
};

auto main() -> int32_t
{
	test0 bobux = { 4, 5, 6.7f, 'd', {{5, 4, 3, 2}, {"cc", "dd"}, "homofobo" , {7, 8, 9}}, {3, 4} };
	auto data = BinaryLove3::serialize(bobux);
	
	test0 bobux1;
	BinaryLove3::deserialize(data, bobux1);
	return int32_t(0);
}