#include <atma/unit_test.hpp>

#include <atma/arena_allocator.hpp>



SCENARIO("arena allocator can be constructed", "[allocators]")
{
	GIVEN("a default-constructed arena-allocator")
	{
		atma::arena_allocator_t A{4096, 64, std::pmr::get_default_resource()};
		void* memory = A.allocate(20);
		void* memory2 = A.allocate(20);
	}
}