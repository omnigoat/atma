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

uint32 const maxnum = 10000;

void alloc_number(queue_t& Q)
{
	for (; counter < maxnum;)
	{
		int sz = 8; //std::max(4, rand() % 16);
		
		Q.with_allocation(sz, [](auto& A) {
			//std::cout << "A: " << counter.load() << std::endl;
			A.encode_uint32(counter++);
		});
	}
}

void read_number(queue_t& Q, numbers_t& ns)
{
	for (; !read_terminate;)
	{
		Q.with_consumption([&](queue_t::decoder_t& D) {
			uint32 r;
			D.decode_uint32(r);
			//std::cout << "r: " << r << std::endl;
			++ns[r];

			if (r == maxnum)
				read_terminate = true;
		}); 
	}
}

SCENARIO("mpsc_queue is amazin")
{
	std::cout << "beginning queue test" << std::endl;

	atma::mpsc_queue_t<false> Q{64};

	uint64 const write_thread_count = 2;
	uint64 const read_thread_count = 2;

	std::vector<numbers_t> readnums{read_thread_count};
	std::vector<std::thread> write_threads;
	std::vector<std::thread> read_threads;

	for (int i = 0; i != write_thread_count; ++i)
		write_threads.emplace_back(alloc_number, std::ref(Q));

	for (int i = 0; i != read_thread_count; ++i)
		read_threads.emplace_back(read_number, std::ref(Q), std::ref(readnums[i]));

	for (int i = 0; i != write_thread_count; ++i)
		write_threads[i].join();

	for (int i = 0; i != read_thread_count; ++i)
		read_threads[i].join();

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
}

