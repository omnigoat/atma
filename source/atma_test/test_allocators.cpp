#include <atma/unit_test.hpp>

#include <atma/arena_allocator.hpp>



SCENARIO("arena allocator can be constructed", "[allocators]")
{
	GIVEN("a default-constructed arena-allocator")
	{
		atma::arena_allocator_t A{64, 8, std::pmr::get_default_resource()};
		void* M1 = A.allocate(20);
		void* M2 = A.allocate(20);
		void* M3 = A.allocate(20);
		void* M4 = A.allocate(20);
	}
}