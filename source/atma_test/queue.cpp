#include <atma/unit_test.hpp>

#include <atma/mpsc_queue.hpp>

#include <string>
#include <thread>
#include <atomic>
#include <mutex>


using queue_t = atma::mpsc_queue_t<false>;
using numbers_t = std::map<uint32, uint32>;

std::atomic<uint32> counter;
std::atomic<bool> read_terminate{false};
std::mutex mtx;

uint32 const maxnum = 100000;

void write_number(queue_t& Q)
{
	for (;;)
	{
		int sz = std::max(4, rand() % 16);
		
		auto idx = counter++;
		if (idx >= maxnum)
			break;

		Q.with_allocation(sz, [idx](auto& A) {
			A.encode_uint32(idx);
		});
	}
}

void read_number(queue_t& Q, numbers_t& ns, uint32* allread)
{
	for (; *allread != maxnum;)
	{
		Q.with_consumption([&](queue_t::decoder_t& D)
		{
			uint32 r;
			D.decode_uint32(r);
			
			if (++ns[r] > 1)
				ATMA_HALT("bad things");

			if (r % 10000 == 0)
				std::cout << "r: " << r << std::endl;

			atma::atomic_pre_increment(allread);
		});
	}
}

SCENARIO("mpsc_queue is amazing")
{
	std::cout << "beginning queue test" << std::endl;

	atma::mpsc_queue_t<false> Q{16 * 1024};

	uint64 const write_thread_count = 4;
	uint64 const read_thread_count = 4;

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

	std::cout << "ended queue alloc/read" << std::endl;
	std::cout << "beginning verification" << std::endl;
#if 1
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
}

