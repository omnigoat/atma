#pragma once

#include <atma/config/platform.hpp>
#include <atma/preprocessor.hpp>

#include <boost/preprocessor.hpp>

#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>

import atma.meta;


namespace atma::bench
{
	struct abstract_scenario
	{
		abstract_scenario();

		virtual void execute_all() = 0;
	};
}

namespace atma::bench::detail
{
	extern std::vector<abstract_scenario*> scenarios_;
}

namespace atma::bench
{
	inline void execute_all()
	{
		for (auto* scenario : detail::scenarios_)
		{
			scenario->execute_all();
		}
	}
}



#define ATMA_BENCH_INTERNAL_TEMPLATE_ARGS_M(r, data, i, elem) BOOST_PP_COMMA_IF(i) typename BOOST_PP_CAT(Param, BOOST_PP_INC(i))
#define ATMA_BENCH_INTERNAL_SCENARIO_EXECUTE_TEMPLATE_ARGS(...) \
	BOOST_PP_SEQ_FOR_EACH_I(ATMA_BENCH_INTERNAL_TEMPLATE_ARGS_M, ~, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define ATMA_BENCH_INTERNAL_SCENARIO(scenario, ...) \
	struct scenario : ::atma::bench::base_scenario<scenario, __VA_ARGS__> \
	{ \
		template <ATMA_BENCH_INTERNAL_SCENARIO_EXECUTE_TEMPLATE_ARGS(__VA_ARGS__)> \
		void execute(); \
	}; \
	static scenario ATMA_PP_CAT(scenario, _instance); \
	template <ATMA_BENCH_INTERNAL_SCENARIO_EXECUTE_TEMPLATE_ARGS(__VA_ARGS__)> \
	void scenario::execute()

#define ATMA_BENCH_SCENARIO(name, ...) \
	ATMA_BENCH_INTERNAL_SCENARIO(ATMA_PP_CAT(name, __LINE__), __VA_ARGS__)

#define ATMA_INTERNAL_BENCH_2(name, benchmark, file, line, markername) \
	if (auto benchmark = this->register_benchmark(name, file, line)) \
		for (auto markername = benchmark.get()->mark(); markername.epochs_remaining(); markername.update()) \
			for (auto _ : markername.execute_epoch())
			//for (size_t atma_bench_i = markername.current_epoch_iterations(); atma_bench_i != 0; atma_bench_i--)

#define ATMA_INTERNAL_BENCH(name, benchmark) \
	ATMA_INTERNAL_BENCH_2(name, benchmark, __FILE__, __LINE__, ATMA_PP_CAT(benchmark, _marker))

#define ATMA_BENCHMARK(name) ATMA_INTERNAL_BENCH(name, ATMA_PP_CAT(abmi, __LINE__))
