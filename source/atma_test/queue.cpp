#include <atma/unit_test.hpp>

#include <atma/mpsc_queue.hpp>

#include <string>

SCENARIO("mpsc_queue is amazin")
{
	atma::mpsc_queue_t<false> Q{512};

	auto A = Q.allocate(80, 4, false);
	A.encode_byte('c');
	Q.commit(A);

	std::string hooray = "hooray";
	Q.with_allocation(80, [&](auto&& A) {
		A.encode_byte('c');
	});
}

