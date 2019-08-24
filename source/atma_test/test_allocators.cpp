#include <cstdlib>

#include <atma/unit_test.hpp>

#include <atma/arena_allocator.hpp>








SCENARIO_OF("allocators", "arena allocator can be constructed")
{
	GIVEN("a default-constructed arena-allocator")
	{
		atma::arena_memory_resource_t B{1024, 8, 64};
		atma::arena_memory_resource_t A{16, 64, 64, &B};

		void* M1 = A.allocate(20);
		void* M2 = A.allocate(20);
		void* M3 = A.allocate(20);
		void* M4 = A.allocate(20);

		A.deallocate(M4, 20);
		A.deallocate(M1, 20);
		A.deallocate(M3, 20);
		A.deallocate(M2, 20);
	}
}