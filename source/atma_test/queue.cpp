#include <atma/unit_test.hpp>

#include <atma/lockfree_queue.hpp>
#include <atma/threading.hpp>
#include <atma/function.hpp>

#include <string>
#include <thread>
#include <atomic>
#include <mutex>


using queue_t = atma::lockfree_queue_t;
using numbers_t = std::map<uint32, uint32>;

std::atomic<uint32> counter;
std::atomic<bool> read_terminate{false};
std::mutex mtx;

#define DO_VERIFICATION 0

#if DO_VERIFICATION
uint32 const maxnum = 100000; //std::numeric_limits<uint32>::max() / 8 + 3; // 1024 * 1024 * 1024; //1000000;
#else
uint32 const maxnum = 2'000'000;
#endif

using fn_t = atma::function<void()>;

void write_number(queue_t& Q)
{
	atma::this_thread::set_debug_name("write-thread");

	for (;;)
	{
		int sz = sizeof(fn_t); //std::max(4, rand() % 32);
		
		auto idx = counter++;
		if (idx >= maxnum)
			break;

		Q.with_allocation(sz, 0, true, [idx](auto& A) {
			A.encode_struct(fn_t{[idx]{std::cout << idx << std::endl; }});
		});
	}
}

void read_number(queue_t& Q, numbers_t& ns, uint32* allread)
{
	atma::this_thread::set_debug_name("read-thread");

	for (; *allread != maxnum;)
	{
		Q.with_consumption([&](queue_t::decoder_t& D)
		{
			uint32 r;
			D.decode_uint32(r);
			
#if DO_VERIFICATION
			if (++ns[r] > 1)
				ATMA_HALT("bad things");
#endif

			if (r % 10000 == 0)
				std::cout << "r: " << r << std::endl;

			atma::atomic_pre_increment(allread);
		});
	}
}

SCENARIO("lockfree_queue is amazing")
{
#if 0
	std::cout << "beginning queue test" << std::endl;

	atma::lockfree_queue_t Q{8 + 512};

	uint64 const write_thread_count = 3;
	uint64 const read_thread_count = 3;

	std::vector<std::thread> write_threads;
	std::vector<std::thread> read_threads;
	std::vector<numbers_t> readnums{read_thread_count};
	uint32 allread = 0;

	for (int i = 0; i != write_thread_count; ++i)
		write_threads.emplace_back(write_number, std::ref(Q));

	for (int i = 0; i != read_thread_count; ++i)
		read_threads.emplace_back(read_number, std::ref(Q), std::ref(readnums[i]), &allread);

	for (int i = 0; i != write_thread_count; ++i)
		write_threads[i].join();

	for (int i = 0; i != read_thread_count; ++i)
		read_threads[i].join();

#if DO_VERIFICATION
	std::cout << "ended queue alloc/read" << std::endl;
	std::cout << "beginning verification" << std::endl;

	for (int i = 0; i != maxnum; ++i)
	{
		int found = 0;
		for (auto const& p : readnums)
		{
			auto candidate = p.find(i);
			if (candidate != p.end())
			{
				ATMA_ASSERT(candidate->second == 1);
				++found;
			}
		}

		ATMA_ASSERT(found == 1);
	}
#endif
#endif
}

