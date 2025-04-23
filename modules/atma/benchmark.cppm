module;

#include <atma/benchmark.hpp>

#include <string>
#include <chrono>

#include <ranges>


#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#  include <xmmintrin.h>
#  undef min
#  undef max

#include <Psapi.h>
#include <cstdint>
#include <cstdio>
#include <profileapi.h>

#include <atma/assert.hpp>
#include <atma/benchmark.hpp>
#include <atma/ranges/zip.hpp>

export module atma.benchmark;


export namespace atma::bench
{
	template <size_t N>
	struct string_literal
	{
		constexpr string_literal(const char (&str)[N])
		{
			std::copy_n(str, N, data);
		}

		char data[N];
	};
}


namespace atma::bench
{
	struct result_t
	{
		// heading : value
		std::map<std::string, std::string> axes;

		std::chrono::nanoseconds time;
		uint64_t iterations;
	};
}

namespace atma::bench
{
	struct result_recorder_t
	{
		virtual void set_axes_count(uint64_t) {}
		virtual void set_axis_header(uint64_t index, std::string_view) {}
		virtual void set_axis_value(uint64_t index, std::string_view) {}

		virtual void record(result_t const&) = 0;
	};
}

export namespace atma::bench
{
	struct result_outputter_t
	{
		virtual ~result_outputter_t() = default;
		virtual void output(result_t const&) = 0;
	};

	struct stdout_outputter_t : result_outputter_t
	{
		virtual void output(result_t const& r) final
		{
			for (auto const& [k, v] : r.axes)
				std::cout << k << ", " << v << ", ";
			std::cout << "mean time: " << (r.time / r.iterations) << std::endl;
		}
	};
}

namespace atma::bench
{
	constexpr int max_outputters = 4;

	using result_outputter_ptr = std::unique_ptr<result_outputter_t>;

	result_outputter_ptr outputters_[max_outputters];

	void scenario_output(result_t const& r)
	{
		for (auto& outputter : outputters_)
		{
			if (outputter)
				outputter->output(r);
		}
	}
}

namespace atma::bench
{
	template <typename... Outputs>
	void set_scenario_output_impl(int index)
	{
		for (int i = index; i != max_outputters; ++i)
			outputters_[i].reset();
	}

	template <typename First, typename... Outputs>
	void set_scenario_output_impl(int index, First&& o, Outputs... outputs)
	{
		ATMA_ASSERT(index < max_outputters);
		outputters_[index] = std::make_unique<std::remove_reference_t<First>>(std::move(o));

		set_scenario_output_impl(index + 1, outputs...);
	}
}

export namespace atma::bench
{
	template <typename... Outputs>
	void set_scenario_output(Outputs... outputs)
	{
		set_scenario_output_impl(0, std::forward<Outputs>(outputs)...);
	}
}

export namespace atma::bench
{
	struct abstract_scenario
		: result_recorder_t
	{
		abstract_scenario();

		virtual void measure_all() = 0;

		virtual void record(result_t const& r) override
		{
			results_.push_back(r);
			//std::cout << "time: " << r.time << ", iters: " << r.iterations << std::endl;
			std::cout << "mean time: " << (r.time / r.iterations) << std::endl;
		}

	private:
		std::vector<result_t> results_;
	};
}

namespace atma::bench::detail
{
	extern std::vector<abstract_scenario*> scenarios_;
}

export namespace atma::bench
{
	inline void measure_all()
	{
		for (auto* scenario : detail::scenarios_)
		{
			scenario->measure_all();
		}
	}

	template <string_literal Name, typename... Params>
	struct axis
	{
		using params_type = atma::meta::list<Params...>;

		static constexpr auto name = Name.data;
		static constexpr auto params = atma::meta::list<Params...>{};
	};

	template <typename... Axis>
	void measure_along() {}
}



namespace atma::bench::detail
{
	using clock_type = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
		std::chrono::high_resolution_clock,
		std::chrono::steady_clock>;

	std::chrono::nanoseconds get_resolution()
	{
		constexpr int iterations = 20;

		clock_type::duration shortest = clock_type::duration::max();
		for (int i = 0; i != iterations; ++i)
		{
			clock_type::time_point const e = clock_type::now();
			clock_type::time_point e2;
			do { e2 = clock_type::now(); } while (e2 == e);
			shortest = std::min(shortest, e2 - e);
		}

		return std::chrono::duration_cast<std::chrono::nanoseconds>(shortest);
	}
}


namespace atma::bench
{
	struct benchmark_signature
	{
		constexpr benchmark_signature() = default;

		constexpr benchmark_signature(char const* name, char const* file, int line, uintptr_t id)
			: name(name), file(file), line(line), id{id}
		{}

		char const* name{};
		char const* file{};
		int line{};
		uintptr_t id{};
	};

	template <typename R, typename C>
	struct base_sso_comparator
	{
		using member_ptr_type = R(C::*);

		base_sso_comparator(member_ptr_type member)
			: member{member}
		{}

	protected:
		member_ptr_type const member;
	};

	template <typename C>
	struct sso_strcmp : base_sso_comparator<char const*, C>
	{
		using base_sso_comparator<char const*, C>::base_sso_comparator;

		template <typename A, typename B>
		int operator () (A const& lhs, B const& rhs) const
		{
			return strcmp((lhs.*this->member), (rhs.*this->member));
		}
	};

	template <typename C>
	sso_strcmp(char const* (C::*)) -> sso_strcmp<C>;

	template <typename R, typename C>
	struct sso_sub : base_sso_comparator<R, C>
	{
		using base_sso_comparator<R, C>::base_sso_comparator;

		template <typename A, typename B>
		int operator () (A const& lhs, B const& rhs) const
		{
			return (int)(lhs.*this->member) - (int)(rhs.*this->member);
		}
	};

	template <typename A, typename B>
	auto spaceship_chain(A const&, B const&)
	{
		return 0;
	}

	template <typename A, typename B, typename C, typename... Comparators>
	auto spaceship_chain(A const& lhs, B const& rhs, C comparator, Comparators... comparators)
	{
		if (auto const r = comparator(lhs, rhs); r == 0)
			return spaceship_chain(lhs, rhs, comparators...);
		else
			return r;
	}

	inline int operator <=> (benchmark_signature const& lhs, benchmark_signature const& rhs)
	{
		return spaceship_chain(lhs, rhs,
			sso_strcmp{&benchmark_signature::name},
			sso_strcmp{&benchmark_signature::file},
			sso_sub{&benchmark_signature::line},
			sso_sub{&benchmark_signature::id});
	}
}


//
// executing_epoch_t
// -------------------
//
namespace atma::bench
{
	struct executing_epoch_t
	{
		struct iterator
		{
			uint64_t i;

			uint64_t operator*() const { return i; }
			void operator ++() {++i;}
			bool operator != (iterator rhs) { return i != rhs.i; }
		};

		uint64_t iters;

		auto begin() const -> iterator { return {0}; }
		auto end() const -> iterator { return {iters}; }
	};
}


//
// executing_benchmark_t
// -----------------------
//
namespace atma::bench
{
	struct benchmark_t;

	struct executing_benchmark_t
	{
		enum class state_t { spinning_up, measuring };

		executing_benchmark_t(benchmark_t*);
		~executing_benchmark_t();

		size_t epochs_remaining() const;
		auto execute_epoch() -> executing_epoch_t;

		void update(detail::clock_type::time_point = detail::clock_type::now());

	private:
		size_t estimate_best_iter_count(std::chrono::nanoseconds elapsed, uint64_t iterations) const;

	private:
		benchmark_t* benchmark_{};

		std::chrono::nanoseconds clock_resolution_;
		std::chrono::nanoseconds target_epoch_duration_;

		// iteration logic
		state_t state_{};
		size_t epoch_iters_{};
		detail::clock_type::time_point time_start_;

		// accumulation
		size_t total_iterations_{};
		std::chrono::nanoseconds total_elapsed_{};
		size_t total_epochs_{};
	};
}


//
// benchmark_t
// -------------
//
namespace atma::bench
{
	struct benchmark_t
	{
		constexpr benchmark_t(result_recorder_t* rr)
			: result_recorder_{rr}
		{}

		bool begin()
		{
			return !executed && (executed = true);
		}
		
		void end()
		{
			//std::cout << "time: " << time << ", iters: " << iterations << std::endl;
			//std::cout << "mean time: " << (time / iterations) << std::endl;
			result_recorder_->record(result_t{.time = time, .iterations = iterations});
		}

		executing_benchmark_t mark()
		{
			// perform confidence analysis here
			return executing_benchmark_t{this};
		}

		//using results_callback_fnptr = void(*)(result_t const&);

		// results
		//results_callback_fnptr results_callback_;
		result_recorder_t* result_recorder_{};

		// config
		size_t epochs{11};
		size_t clock_multiplier{1000};
		std::chrono::nanoseconds min_epoch_duration{std::chrono::milliseconds(1)};
		std::chrono::nanoseconds max_epoch_duration{std::chrono::milliseconds(100)};
		size_t min_epoch_iterations{1};
		size_t epoch_iterations{};

		// accumulation
		std::chrono::nanoseconds time{};
		size_t iterations{};
		size_t cycles{};
		size_t branch_instructions{};
		size_t branch_misses{};
		size_t cache_misses{};

		bool executed{};
	};
}


//
// benchmark_handle
// ------------------
//
namespace atma::bench
{
	struct benchmark_handle
	{
		constexpr benchmark_handle() = default;
		constexpr benchmark_handle(benchmark_t* bm)
			: benchmark_{bm}
		{}

		~benchmark_handle()
		{
			if (benchmark_)
			{
				benchmark_->end();
			}
		}

		operator bool() const
		{
			return benchmark_ != nullptr;
		}

		executing_benchmark_t execute() const
		{
			return benchmark_->mark();
		}

		benchmark_t* get() const { return benchmark_; }

	private:
		benchmark_t* benchmark_{};
	};
}


//
// executing_benchmark_t implementation
//
namespace atma::bench
{
	
	__forceinline executing_benchmark_t::executing_benchmark_t(benchmark_t* bm)
		: benchmark_{bm}
		, clock_resolution_{detail::get_resolution()}
		, target_epoch_duration_{benchmark_->clock_multiplier * clock_resolution_}
		, epoch_iters_{benchmark_->min_epoch_iterations}
	{
		if (target_epoch_duration_ < benchmark_->min_epoch_duration)
			target_epoch_duration_ = benchmark_->min_epoch_duration;
		if (target_epoch_duration_ > benchmark_->max_epoch_duration)
			target_epoch_duration_ = benchmark_->max_epoch_duration;
	}

	__forceinline executing_benchmark_t::~executing_benchmark_t()
	{
		benchmark_->time += total_elapsed_;
		benchmark_->iterations += total_iterations_;
	}

	inline size_t executing_benchmark_t::epochs_remaining() const
	{
		ATMA_ASSERT(total_epochs_ <= benchmark_->epochs);
		return benchmark_->epochs - total_epochs_;
	}

	inline auto executing_benchmark_t::execute_epoch() -> executing_epoch_t
	{
		time_start_ = detail::clock_type::now();
		return executing_epoch_t{epoch_iters_};
	}

	inline size_t executing_benchmark_t::estimate_best_iter_count(std::chrono::nanoseconds elapsed, uint64_t iterations) const
	{
		auto const delapsed = (double)elapsed.count();
		auto const dtarget_duration = (double)target_epoch_duration_.count();
		auto const dremaining_iters = std::max(dtarget_duration, 0.0) / delapsed * (double)epoch_iters_;

		return static_cast<size_t>(dremaining_iters * 1.2 + 0.5);
	}

	__forceinline void executing_benchmark_t::update(detail::clock_type::time_point time_end)
	{
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(time_end - time_start_);
		if (elapsed == std::chrono::nanoseconds::zero())
			elapsed = clock_resolution_;

		if (state_ == state_t::spinning_up)
		{
			// two-thirds or more of the way to the target epoch duration
			bool const close_enough = elapsed * 3 >= target_epoch_duration_ * 2;

			// yeah goood enough - consider this a valid epoch and add it
			// to our measurements - but still reevaluate the number of iterations
			if (close_enough)
			{
				state_ = state_t::measuring;
				total_iterations_ += epoch_iters_;
				total_elapsed_ += elapsed;
				epoch_iters_ = estimate_best_iter_count(total_elapsed_, total_iterations_);
				++total_epochs_;
			}
			// we're very far away from the target duration, x10 the iterations
			else if (elapsed * 10 < target_epoch_duration_)
			{
				epoch_iters_ = (epoch_iters_ * 10 > epoch_iters_)
					? estimate_best_iter_count(elapsed, epoch_iters_)
					: 0;
				//std::cout << "new iters: " << epoch_iters_ << std::endl;
			}
			else
			{
				epoch_iters_ = estimate_best_iter_count(elapsed, epoch_iters_);
				//std::cout << "new iters: " << epoch_iters_ << std::endl;
			}
		}
		else
		{
			total_iterations_ += epoch_iters_;
			total_elapsed_ += elapsed;
			epoch_iters_ = estimate_best_iter_count(total_elapsed_, total_iterations_);
			++total_epochs_;
		}

		if (total_epochs_ == benchmark_->epochs)
		{
			epoch_iters_ = 0;
		}
	}
}

#if 0
namespace std
{
	template <>
	struct hash<atma::bench::benchmark_signature>
	{
		size_t operator()(atma::bench::benchmark_signature const& x) const
		{
			return x.line;
		}
	};
}
#endif

namespace atma::bench
{
	struct scenario_recorder_t
		: result_recorder_t
	{
		virtual void set_axes_count(uint64_t count)
		{
			axes.clear();
			axes.resize(count);
		}

		virtual void set_axis_header(uint64_t index, std::string_view header)
		{
			axes[index].first = header;
		}

		virtual void set_axis_value(uint64_t index, std::string_view value)
		{
			axes[index].second = value;
		}

		virtual void record(result_t const& r) override
		{
			result_t rr = r;
			for (auto const& [k, v] : axes)
				rr.axes.emplace(k, v);
			//std::cout << "mean time, " << (r.time / r.iterations) << std::endl;
			scenario_output(rr);
		}

		std::vector<std::pair<std::string, std::string>> axes;
	};
}

namespace atma::bench
{
	template <typename... Args>
	void invoke_expand_i(auto&& f)
	{
		uint64_t index = 0;
		(std::invoke(f, index++, Args{}), ...);
	}

	template <typename S, typename... Axis>
	struct base_scenario
		: abstract_scenario
	{
		using axes_type = meta::list<Axis...>;
		using combinations = meta::select_combinations_t<typename Axis::params_type...>;

		base_scenario()
		{
			recorder_.set_axes_count(axes_type::size);

			invoke_expand_i<Axis...>(
				[&](uint64_t index, auto a) {
					recorder_.set_axis_header(index, a.name);
				});
		}

		void measure_all() override
		{
			std::invoke(
				[&]<typename... Combos>(meta::list<Combos...>) {
					((std::invoke([&]<typename... Axes>(meta::list<Axes...>) {
						this->execute_wrapper<Axes...>();
					}, Combos{})), ...);
				},
				combinations{}
			);
		}

		template <typename... axis>
		void execute_wrapper()
		{
			invoke_expand_i<axis...>(
				[&](uint64_t index, auto a)
				{
					recorder_.set_axis_value(index++, a.name);
				});

			do
			{
				executed_benchmark_this_run_ = false;
				static_cast<S*>(this)->template execute<axis...>();
			} while (executed_benchmark_this_run_);
		}

		benchmark_handle register_benchmark(char const* name, char const* file, int line, uintptr_t id)
		{
			if (executed_benchmark_this_run_)
			{
				return benchmark_handle{};
			}
			else if (auto [it, inserted] = benchmarks_.emplace(benchmark_signature(name, file, line, id), benchmark_t{&recorder_}); inserted)
			{
				executed_benchmark_this_run_ = true;
				return benchmark_handle{&it->second};
			}
			else
			{
				return benchmark_handle{};
			}
		}

	private:
		scenario_recorder_t recorder_;

		std::vector<std::string> axis_headers;
		std::vector<std::string> current_axis;

	protected:
		using benchmarks_t = std::map<benchmark_signature, benchmark_t>;
		using results_t = std::vector<result_t>;
		
		benchmarks_t benchmarks_;
		results_t results_;
		bool executed_benchmark_this_run_{};
	};
}














































export namespace atma::bench::detail
{
	std::vector<abstract_scenario*> scenarios_;
}

namespace atma::bench
{
	abstract_scenario::abstract_scenario()
	{
		detail::scenarios_.push_back(this);
	}
}



namespace atma::bench
{
	template <string_literal name, typename payload>
	struct param : payload
	{
		constexpr static inline auto name = name.data;
	};

	template <typename Key, typename Value>
	struct key_value_param_payload
	{
		using key_type = Key;
		using value_type = Value;

		static inline const auto default_key = Key{};
		static inline const auto default_value = Value{};
	};

	template <string_literal name, typename Key, typename Value>
	struct key_value_param
		: param<name, key_value_param_payload<Key, Value>>
	{ };
}

using hash_map_kv_pairs = atma::bench::axis<"types",
	atma::bench::key_value_param<"u64|u64", uint64_t, uint64_t>,
	atma::bench::key_value_param<"u64|string", uint64_t, std::string>
>;


struct hash_map_adaptor_1
{
	template <typename K, typename V>
	using type = std::map<K, V>;
};

struct hash_map_adaptor_2
{
	template <typename K, typename V>
	using type = std::unordered_map<K, V>;
};

using hash_map_adaptors = atma::bench::axis<"implementation",
	atma::bench::param<"map1", hash_map_adaptor_1>,
	atma::bench::param<"map2", hash_map_adaptor_2>>;


ATMA_BENCH_SCENARIO(hash_map, hash_map_adaptors, hash_map_kv_pairs)
{
	using hash_map_type = typename Param1::template type<
		typename Param2::key_type,
		typename Param2::value_type>;

	auto default_key = Param2::default_key;
	auto const default_value = Param2::default_value;

	hash_map_type hash_map;

	ATMA_BENCHMARK("insert")
	{
		hash_map[default_key] = default_value;
	}

	ATMA_BENCHMARK("erase")
	{
		hash_map.erase(default_key);
	}
}

















































































// win32 implementation


//export module atma.bench;

using NTSTATUS = long;

import atma.types;



export namespace atma::bench
{
	template <int Line, typename Expr>
	inline __forceinline void no_optimize(Expr&& expr)
	{
		static char const volatile* volatile _ = &reinterpret_cast<char const volatile&>(expr);
	}
}



export typedef enum _KPROFILE_SOURCE
{
    ProfileTime                     = 0x00,
    ProfileTotalIssues              = 0x02,
    ProfileBranchInstructions       = 0x06,
    ProfileCacheMisses              = 0x0A,
    ProfileBranchMispredictions     = 0x0B,
    ProfileTotalCycles              = 0x13,
    ProfileUnhaltedCoreCycles       = 0x19,
    ProfileInstructionRetired       = 0x1A,
    ProfileUnhaltedReferenceCycles  = 0x1B,
    ProfileLLCReference             = 0x1C,
    ProfileLLCMisses                = 0x1D,
    ProfileBranchInstructionRetired = 0x1E,
    ProfileBranchMispredictsRetired = 0x1F,
} KPROFILE_SOURCE, *PKPROFILE_SOURCE;


typedef NTSYSAPI NTSTATUS NTAPI F_NtCreateProfile(
    OUT PHANDLE        ProfileHandle,
    IN HANDLE          Process OPTIONAL,
    IN PVOID           ImageBase,
    IN ULONG           ImageSize,
    IN ULONG           BucketSize,
    IN PVOID           Buffer,
    IN ULONG           BufferSize,
    IN KPROFILE_SOURCE ProfileSource,
    IN KAFFINITY       Affinity
);

typedef NTSYSAPI NTSTATUS NTAPI F_NtStartProfile(
    IN HANDLE           ProfileHandle
);

typedef NTSYSAPI NTSTATUS NTAPI F_NtStopProfile(
    IN HANDLE           ProfileHandle
);

using ZwCreateProfileEx_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
    OUT PHANDLE        ProfileHandle,
    IN HANDLE          Process OPTIONAL,
    IN PVOID           ImageBase,
    IN ULONG           ImageSize,
    IN ULONG           BucketSize,
    IN PVOID           Buffer,
    IN ULONG           BufferSize,
    IN KPROFILE_SOURCE ProfileSource,
    IN USHORT          GroupCount,
	IN PGROUP_AFFINITY GroupAffinity);

using ZwStartProfile_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
	IN HANDLE ProfileHandle);

using ZwStopProfile_fnptr_type = NTSYSAPI NTSTATUS (NTAPI*)(
	IN HANDLE ProfileHandle);


HMODULE GetCurrentModule()
{
    HMODULE module = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) GetCurrentModule, &module);
    return module;
}

#define DO_NOT_OPTIMIZE(var) 
#define SUM_COUNT (100 * 1000 * 1000)

// expected count of cycles for loop when testing IPC
#define CYCLE_COUNT (1000 * 1000 * 1000)

static uint8_t arr[SUM_COUNT];

static void SumArray(void* Arg)
{
	uint64_t result = 0;
	DO_NOT_OPTIMIZE(result);

	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		if (arr[i] < 128)
		{
			DO_NOT_OPTIMIZE(result); // force compiler to generate actual branch
			result += arr[i];
		}
	}

	DO_NOT_OPTIMIZE(result);
}

// sorts array with counting sort
static void SortArray(void)
{
	size_t offset = 0;
	uint32_t counts[256] = { 0 };
	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		counts[arr[i]]++;
	}
	for (size_t i = 0; i < 256; i++)
	{
		for (size_t c = 0; c < counts[i]; c++)
		{
			arr[offset++] = (uint8_t)i;
		}
	}
}

// fill array with random values
static void RandomizeArray(void)
{
	uint32_t random = 1;
	for (size_t i = 0; i < SUM_COUNT; i++)
	{
		arr[i] = (uint8_t)(random >> 24);
		random = 0x01000193 * random + 0x811c9dc5;
	}
}

export struct WinNtModuleContext
{
	WinNtModuleContext()
		: process_{GetCurrentProcess()}
		, module_{GetCurrentModule()}
	{
		HMODULE nt = LoadLibraryA("ntdll.dll");

		create_profile = (ZwCreateProfileEx_fnptr_type)GetProcAddress(nt, "ZwCreateProfileEx");
		start_profile = (ZwStartProfile_fnptr_type)GetProcAddress(nt, "ZwStartProfile");
		stop_profile = (ZwStopProfile_fnptr_type)GetProcAddress(nt, "ZwStopProfile");

		MODULEINFO module_info;
		if (BOOL module_info_success = GetModuleInformation(process_, module_, &module_info, sizeof(module_info)))
		{
			image_base_ = module_info.lpBaseOfDll;
			image_size_ = module_info.SizeOfImage;
		}
		else
		{
			// assert properly here
			printf("GetModuleInformation() failed!\n");
		}
	}

	HANDLE process() const { return process_; }
	HMODULE module() const { return module_; }

	void* image_base() const { return image_base_; }
	DWORD image_size() const { return image_size_; }

	ZwCreateProfileEx_fnptr_type create_profile;
	ZwStartProfile_fnptr_type start_profile;
	ZwStopProfile_fnptr_type stop_profile;

private:
	HANDLE process_{};
	HMODULE module_{};

	void* image_base_{};
	DWORD image_size_{};
};

export struct WinNtProfileSession
{
	WinNtProfileSession(WinNtModuleContext& ctx)
		: ctx_{ctx}
	{
		if (!ctx_.image_base())
		{
			// no valid address to instrument
			return;
		}

		// find largest power of two that fits image_size
		for (auto i = ctx_.image_size(); i > 4; i >>= 1)
			++pow2base_;
			
		create_profile(total_cycles_handle_, total_cycles_, ProfileTotalCycles);
		create_profile(branches_handle_, branches_, ProfileBranchInstructions);
		create_profile(branches_mispredicted_handle_, branches_mispredicted_, ProfileBranchMispredictions);
		create_profile(cache_misses_handle_, cache_misses_, ProfileCacheMisses);
	}

private:
	void create_profile(HANDLE& handle, ULONG& bucket, KPROFILE_SOURCE source)
	{
		[[maybe_unused]] NTSTATUS create_status = ctx_.create_profile(&handle, 
			ctx_.process(), ctx_.image_base(), ctx_.image_size(), 
			pow2base_, &bucket, sizeof(ULONG), 
			source,
			0, nullptr);

		ATMA_ASSERT(create_status == 0);
	}

private:
	WinNtModuleContext& ctx_;
	ULONG pow2base_{2};

	// handles
	HANDLE total_cycles_handle_{};
	HANDLE branches_handle_{};
	HANDLE branches_mispredicted_handle_{};
	HANDLE cache_misses_handle_{};

	// buckets
	ULONG total_cycles_{};
	ULONG branches_{};
	ULONG branches_mispredicted_{};
	ULONG cache_misses_{};
};

#if 0
ATMA_BENCHMARK_SUITE("hash-maps")
	.add_axis()
{
}
#endif

void test()
{
    HMODULE nt = LoadLibraryA("ntdll.dll");

    F_NtCreateProfile* NtCreateProfile = (F_NtCreateProfile*) GetProcAddress(nt, "NtCreateProfile");
    F_NtStartProfile* NtStartProfile = (F_NtStartProfile*) GetProcAddress(nt, "NtStartProfile");
    F_NtStopProfile* NtStopProfile = (F_NtStopProfile*) GetProcAddress(nt, "NtStopProfile");

	//[[maybe_unused]] auto blah = GetProcAddress(nt, "ZwCreateProfileEx");;

	


    HANDLE process = GetCurrentProcess();
    HMODULE module = GetCurrentModule();
    printf("%p %p\n", process, module);

    MODULEINFO module_info;
    BOOL module_info_success = GetModuleInformation(process, module, &module_info, sizeof(module_info));
    if (!module_info_success)
    {
        printf("GetModuleInformation() failed!\n");
        return;
    }

    void* image_base = module_info.lpBaseOfDll;
    auto image_size = module_info.SizeOfImage;

    printf("%p\n", image_base);
    printf("%d\n", (int) image_size);

    HANDLE profile, profile2;

    ULONG buffer[1] = {};
	ULONG buffer2[1] = {};


	// let's see how many profiles we can create at once
	{
		size_t i = 0;
		for ( ; i != 512; ++i)
		{
			NTSTATUS create_status = NtCreateProfile(&profile, process, image_base, image_size, 30, buffer, 4, ProfileLLCMisses, ULONG_PTR(-1));
			if (create_status != 0)
				break;
		}
		printf("concurrent profiles available: %llu\n", i);
	}

    NTSTATUS create_status = NtCreateProfile(&profile, process, image_base, image_size, 30, buffer, 4, ProfileLLCMisses, ULONG_PTR(-1));
	NTSTATUS create_status2 = NtCreateProfile(&profile2, process, image_base, image_size, 30, buffer2, 4, ProfileBranchMispredictions, ULONG_PTR(-1));
    printf("NtCreateProfile: %x\n", create_status);
	printf("NtCreateProfile: %x\n", create_status2);

    NtStartProfile(profile);
	NtStartProfile(profile2);
    {
        int* matrix = (int*) malloc(sizeof(int) * 16 * 1024 * 16 * 1024);
        for (int i = 0; i < 10000; i++)
            for (int j = 0; j < 10000; j++)
                matrix[i * 16 * 1024 + j] *= i * j;
        free(matrix);
    }
    NtStopProfile(profile);
	NtStopProfile(profile2);
    printf("Cache misses 1: %d\n", buffer[0]);
	printf("Branch mispredictions 1: %d\n", buffer2[0]);
    buffer[0] = 0;
	buffer2[0] = 0;


    NtStartProfile(profile);
	NtStartProfile(profile2);
    {
        int* matrix = (int*) malloc(sizeof(int) * 16 * 1024 * 16 * 1024);
        for (int i = 0; i < 10000; i++)
            for (int j = 0; j < 10000; j++)
                matrix[j * 16 * 1024 + i] *= i * j;
        free(matrix);
    }
    NtStopProfile(profile);
	NtStopProfile(profile2);
    printf("Cache misses 2: %d\n", buffer[0]);
	printf("Branch mispredictions 2: %d\n", buffer2[0]);
    buffer[0] = 0;
	buffer2[0] = 0;


	RandomizeArray();
	buffer2[0] = 0;
	NtStartProfile(profile2);
	{
		SumArray(nullptr);
	}
	NtStopProfile(profile2);
	printf("array - Branch mispredictions 1: %d\n", buffer2[0]);

	SortArray();
	buffer2[0] = 0;
	NtStartProfile(profile2);
	{
		SumArray(nullptr);
	}
	NtStopProfile(profile2);
	printf("array - Branch mispredictions 2: %d\n", buffer2[0]);
}
