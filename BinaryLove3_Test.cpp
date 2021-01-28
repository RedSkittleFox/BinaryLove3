#include <BinaryLove3.hpp>
#include "CppUnitTest.h"

#include <list>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BinaryLove3Test
{
	TEST_CLASS(serialize_deserializ)
	{
	public:
		
		TEST_METHOD(trivial_simple_layout)
		{
			struct simple_layout
			{
				uint32_t v0, v1;
				float_t v2;
				char v3;
				double_t v4;
			};

			simple_layout in{ 0, 1, 3.1f,'d',5.6 };
			std::vector<std::byte> data;
			data = BinaryLove3::serialize(in);

			simple_layout out;
			Assert::IsTrue(BinaryLove3::deserialize(data, out));
			Assert::AreEqual(std::memcmp(&in, &out, sizeof(simple_layout)), 0);
		}

		TEST_METHOD(random_access_container_trivial_data)
		{
			std::vector<uint32_t> in{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

			std::vector<std::byte> data;
			data = BinaryLove3::serialize(in);

			std::vector<uint32_t> out;
			Assert::IsTrue(BinaryLove3::deserialize(data, out));
			Assert::IsTrue(in == out);
		}

		TEST_METHOD(random_access_container_nontrivial_data)
		{
			std::vector<std::string> in{ "A", "Quick", "Brown", "Fox", "Jumps", "Over", "The", "Lazy", "Dog"};

			std::vector<std::byte> data;
			data = BinaryLove3::serialize(in);

			std::vector<std::string> out;
			Assert::IsTrue(BinaryLove3::deserialize(data, out));
			Assert::IsTrue(in == out);
		}

		TEST_METHOD(nonrandom_access_container)
		{
			std::list<uint32_t> in{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

			std::vector<std::byte> data;
			data = BinaryLove3::serialize(in);

			std::list<uint32_t> out;
			Assert::IsTrue(BinaryLove3::deserialize(data, out));
			Assert::IsTrue(in == out);
		}

		TEST_METHOD(compound)
		{
			struct compound
			{
				uint32_t v0;
				uint32_t v1;
				float_t v2;
				char v3;
				struct
				{
					std::vector<int> vec_of_trivial;
					std::vector<std::string> vec_of_nontrivial;
					std::string str;
					std::list<int> non_random_access_container;
				} non_trivial;
				struct
				{
					uint32_t v0;
					uint32_t v1;
				} trivial;
			};

			compound in{ 0, 1, 2.2f, 'd', {{0, 1, 2, 3, 4}, {"Foxese", "Can", "Fly"}, "Foxo in a boxo!", {5, 6, 7, 8, 9}}, {10, 11}};

			std::vector<std::byte> data;
			data = BinaryLove3::serialize(in);

			compound out;
			Assert::IsTrue(BinaryLove3::deserialize(data, out));

			Assert::IsTrue(in.v0 == out.v0);
			Assert::IsTrue(in.v1 == out.v1);
			Assert::IsTrue(in.v2 == out.v2);
			Assert::IsTrue(in.v3 == out.v3);
			Assert::IsTrue(in.non_trivial.vec_of_trivial == out.non_trivial.vec_of_trivial);
			Assert::IsTrue(in.non_trivial.vec_of_nontrivial == out.non_trivial.vec_of_nontrivial);
			Assert::IsTrue(in.non_trivial.str == out.non_trivial.str);
			Assert::IsTrue(in.non_trivial.non_random_access_container == out.non_trivial.non_random_access_container);
			Assert::IsTrue(in.trivial.v0 == out.trivial.v0);
			Assert::IsTrue(in.trivial.v1 == out.trivial.v1);
		}
	};
}
