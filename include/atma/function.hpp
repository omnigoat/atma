#pragma once

#include <atma/bind.hpp>
#include <atma/tuple.hpp>
#include <atma/function_composition.hpp>
#include <atma/assert.hpp>

#include <tuple>
#include <functional>

namespace atma
{
	enum class functor_storage_t
	{
		heap,
		external,
		relative,
	};
}

namespace atma
{
	template <size_t, functor_storage_t, typename>
	struct basic_generic_function_t;
	
	template <size_t BufferSize, typename FN>
	using basic_function_t = basic_generic_function_t<BufferSize, functor_storage_t::heap, FN>;

	template <typename FN>
	using function = basic_generic_function_t<32, functor_storage_t::heap, FN>;

	template <typename FN>
	using external_function = basic_generic_function_t<32, functor_storage_t::external, FN>;

	template <typename FN>
	using relative_function = basic_generic_function_t<32, functor_storage_t::relative, FN>;


	// functor-buf
	
	namespace detail
	{
		// size of buffer to construct functor. make this larger will allow for more
		// data to be stored in the "Small Function Optimization", but each instance
		// of basic_generic_function_t will take up more space.
		template <size_t Bytes>
		struct functor_buf_t
		{
			static size_t const size = Bytes;
			static_assert(size >= sizeof(void*), "basic_generic_function_t-buf needs to be at least the size of a pointer");
			char buf[size];

			functor_buf_t()
				: buf{}
			{}

			functor_buf_t(functor_buf_t<Bytes> const& rhs)
			{
				memcpy(buf, rhs.buf, Bytes);
			}
		};

		template <size_t BS, typename FN>
		constexpr auto enable_SFO() -> bool
		{
			return sizeof(FN) <= BS;
		}

		template <typename R, typename FN, typename... Params>
		constexpr bool result_matches_v = std::is_same_v<R, std::result_of_t<std::remove_reference_t<FN>(Params...)>>;
	}


	// vtable
	namespace detail
	{
		template <size_t BS, typename R, typename... Params>
		struct functor_vtable_t
		{
			using copy_construct_fntype = auto(*)(functor_buf_t<BS>&, void const*) -> void;
			using destruct_fntype       = auto(*)(functor_buf_t<BS>&) -> void;
			using assign_fntype         = auto(*)(functor_buf_t<BS>&, functor_vtable_t<BS, R, Params...> const*, functor_buf_t<BS> const&) -> void;
			using move_fntype           = auto(*)(functor_buf_t<BS>&, functor_vtable_t<BS, R, Params...> const*, functor_buf_t<BS>&&) -> void;
			using exsize_fntype         = auto(*)() -> size_t;
			using target_fntype         = auto(*)(functor_buf_t<BS>&) -> void*;
			using type_info_fntype      = auto(*)() -> std::type_info const&;
			using call_fntype           = auto(*)(functor_buf_t<BS> const&, Params...) -> R;
			using relocate_fntype       = auto(*)(functor_buf_t<BS>&, void*) -> void;
			using mk_vtable_fntype      = auto(*)(functor_storage_t) -> functor_vtable_t*;

			copy_construct_fntype copy_construct;
			destruct_fntype       destruct;
			assign_fntype         assign_into;
			move_fntype           move_into;
			exsize_fntype         external_size;
			target_fntype         target;
			type_info_fntype      type_info;
			call_fntype           call;
			relocate_fntype       relocate;
			mk_vtable_fntype      mk_vtable;
		};

		template <size_t BS, functor_storage_t, typename FN, typename R, typename... Params>
		auto generate_vtable() -> functor_vtable_t<BS, R, Params...>*;
	}


	// constructing doesn't require a vtable
	namespace detail
	{
		template <size_t BS, functor_storage_t FS, typename FN, bool = enable_SFO<BS, FN>()>
		struct constructing_t
		{
			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, RFN&& fn) -> void
			{
				new (&buf) FN{std::forward<RFN>(fn)};
			}

			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, void* exbuf, RFN&& fn) -> void
			{
				new (&buf) FN{std::forward<RFN>(fn)};
			}
		};

		template <size_t BS, typename FN>
		struct constructing_t<BS, functor_storage_t::heap, FN, false>
		{
			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, RFN&& fn) -> void
			{
				reinterpret_cast<FN*&>(buf) = new FN{std::forward<RFN>(fn)};
			}

			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, void* exbuf, RFN&& fn) -> void
			{
				FN*& p = reinterpret_cast<FN*&>(buf);
				p = reinterpret_cast<FN*>(exbuf);
				new (p) FN{std::forward<RFN>(fn)};
			}
		};

		template <size_t BS, typename FN>
		struct constructing_t<BS, functor_storage_t::external, FN, false>
		{
			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, RFN&& fn) -> void
			{
				reinterpret_cast<FN*&>(buf) = new FN{std::forward<RFN>(fn)};
			}

			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, void* exbuf, RFN&& fn) -> void
			{
				FN*& p = reinterpret_cast<FN*&>(buf);
				p = reinterpret_cast<FN*>(exbuf);
				new (p) FN{std::forward<RFN>(fn)};
			}
		};

		template <size_t BS, typename FN>
		struct constructing_t<BS, functor_storage_t::relative, FN, false>
		{
			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, RFN&& fn) -> void
			{
				reinterpret_cast<FN*&>(buf) = new FN{std::forward<RFN>(fn)};
			}

			template <typename RFN>
			static auto construct(functor_buf_t<BS>& buf, void* exbuf, RFN&& fn) -> void
			{
				intptr& ip = reinterpret_cast<intptr&>(buf);
				ip = reinterpret_cast<intptr>(exbuf) - reinterpret_cast<intptr>(&buf);
				FN* p = reinterpret_cast<FN*>(reinterpret_cast<char*>(&buf) + ip);
				new (p) FN{std::forward<RFN>(fn)};
			}
		};
	}


	// vtable-impl
	namespace detail
	{
		template <size_t BS, functor_storage_t, typename FN, bool, typename R, typename... Params>
		struct vtable_impl_t
		{
			static auto copy_construct(functor_buf_t<BS>& buf, void const* fnv) -> void
			{
				new (&reinterpret_cast<FN&>(buf)) FN{*((FN const*)fnv)};
			}

			static auto destruct(functor_buf_t<BS>& fn) -> void
			{
				reinterpret_cast<FN&>(fn).~FN();
			}

			static auto assign_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS> const& src) -> void
			{
				auto sfn = (FN const*)target(const_cast<functor_buf_t<BS>&>(src));
				dvtable->destruct(dest);
				new (dvtable->target(dest)) FN{*sfn};
			}

			static auto move_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS>&& src) -> void
			{
				auto sfn = (FN*)target(src);
				dvtable->destruct(dest);
				new (dvtable->target(dest)) FN{std::move(*sfn)};
			}

			static auto external_size() -> size_t
			{
				// SFO has no external allocations
				return 0u;
			}

			static auto target(functor_buf_t<BS>& buf) -> void*
			{
				auto&& fn = reinterpret_cast<FN&>(buf);
				return &fn;
			}

			static auto type_info() -> std::type_info const&
			{
				return typeid(FN);
			}

			static auto call(functor_buf_t<BS> const& buf, Params... args) -> R
			{
				auto& fn = reinterpret_cast<FN&>(const_cast<functor_buf_t<BS>&>(buf));
				//return fn(std::forward<Params>(args)...);
				return std::invoke(fn, std::forward<Params>(args)...);
			}

			static auto relocate(functor_buf_t<BS>& buf, void*) -> void
			{
				// no relocation required for SFO
			}
		};

		template <size_t BS, typename FN, typename R, typename... Params>
		struct vtable_impl_t<BS, functor_storage_t::heap, FN, false, R, Params...>
		{
			static auto copy_construct(functor_buf_t<BS>& buf, void const* fnv) -> void
			{
				auto fn = reinterpret_cast<FN const*>(fnv);
				reinterpret_cast<FN*&>(buf) = new FN{*fn};
			}

			static auto destruct(functor_buf_t<BS>& buf) -> void
			{
				FN*& p = reinterpret_cast<FN*&>(buf);
				delete p;
			}

			static auto assign_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS> const& src) -> void
			{
				auto sfn = (FN const*)target(const_cast<functor_buf_t<BS>&>(src));
				dvtable->destruct(dest);
				new (dvtable->target(dest)) FN{*sfn};
			}

			static auto move_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS>&& src) -> void
			{
				auto sfn = (FN*)target(src);
				dvtable->destruct(dest);
				new (dvtable->target(dest)) FN{std::move(*sfn)};
			}

			static auto external_size() -> size_t
			{
				return sizeof(FN);
			}

			static auto target(functor_buf_t<BS>& buf) -> void*
			{
				return reinterpret_cast<FN*&>(buf);
			}

			static auto call(functor_buf_t<BS> const& buf, Params... args) -> R
			{
				auto ip = reinterpret_cast<intptr const&>(buf);
				FN* const& p = reinterpret_cast<FN* const&>(ip);
				return (*p)(std::forward<Params>(args)...);
			}

			static auto relocate(functor_buf_t<BS>& buf, void* exbuf) -> void
			{
				// SERIOUSLY, todo
			}
		};

		template <size_t BS, typename FN, typename R, typename... Params>
		struct vtable_impl_t<BS, functor_storage_t::external, FN, false, R, Params...>
		{
			static auto copy_construct(functor_buf_t<BS>& buf, void const* fnv) -> void
			{
				auto fn = reinterpret_cast<FN const*>(fnv);
				new (target(buf)) FN{*fn};
			}

			static auto destruct(functor_buf_t<BS>& buf) -> void
			{
				FN* p = reinterpret_cast<FN*&>(buf);
				p->~FN();
			}

			static auto assign_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS> const& src) -> void
			{
				auto sfn = (FN const*)target(const_cast<functor_buf_t<BS>&>(src));
				dvtable->destruct(dest);
				dvtable->copy_construct(dest, sfn);
			}

			static auto move_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS>&& src) -> void
			{
				auto sfn = (FN*)target(src);
				dvtable->destruct(dest);
				new (dvtable->target(dest)) FN{std::move(*sfn)};
			}

			static auto external_size() -> size_t
			{
				return sizeof(FN);
			}

			static auto target(functor_buf_t<BS>& buf) -> void*
			{
				auto ip = reinterpret_cast<intptr&>(buf);
				FN* p = reinterpret_cast<FN*>(ip);
				return p;
			}

			static auto call(functor_buf_t<BS> const& buf, Params... args) -> R
			{
				auto ip = reinterpret_cast<intptr const&>(buf);
				FN* const& p = reinterpret_cast<FN* const&>(ip);
				return (*p)(std::forward<Params>(args)...);
			}

			static auto relocate(functor_buf_t<BS>& buf, void* exbuf) -> void
			{
				// original external
				auto pfn = (FN*)target(buf);

				// move-construct into new external
				new (exbuf) FN{std::move(*pfn)};

				// destruct original
				destruct(buf);

				// assign external buffer to us
				reinterpret_cast<FN*&>(buf) = reinterpret_cast<FN*>(exbuf);
			}
		};

		template <size_t BS, typename FN, typename R, typename... Params>
		struct vtable_impl_t<BS, functor_storage_t::relative, FN, false, R, Params...>
		{
			static auto copy_construct(functor_buf_t<BS>& buf, void const* fnv) -> void
			{
				auto fn = reinterpret_cast<FN const*>(fnv);
				new (target(buf)) FN{*fn};
			}

			static auto destruct(functor_buf_t<BS>& buf) -> void
			{
				((FN*)target(buf))->~FN();
			}

			static auto assign_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS> const& src) -> void
			{
				auto sfn = (FN const*)target(const_cast<functor_buf_t<BS>&>(src));
				dvtable->destruct(dest);
				new (dvtable->target(dest)) FN{*sfn};
			}

			static auto move_into(functor_buf_t<BS>& dest, functor_vtable_t<BS, R, Params...> const* dvtable, functor_buf_t<BS>&& src) -> void
			{
				auto sfn = (FN*)target(src);
				dvtable->destruct(dest);
				new (dvtable->target(dest)) FN{std::move(*sfn)};
			}

			static auto external_size() -> size_t
			{
				return sizeof(FN);
			}

			static auto target(functor_buf_t<BS>& buf) -> void*
			{
				auto ip = reinterpret_cast<intptr const&>(buf);
				FN* p = reinterpret_cast<FN*>(buf.buf + ip);
				return p;
			}

			static auto call(functor_buf_t<BS> const& buf, Params... args) -> R
			{
				auto ip = reinterpret_cast<intptr const&>(buf);
				FN const* p = reinterpret_cast<FN const*>(buf.buf + ip);
				return (*p)(std::forward<Params>(args)...);
			}

			static auto relocate(functor_buf_t<BS>& buf, void* exbuf) -> void
			{
				// original external
				auto pfn = (FN*)target(buf);

				// move-construct into new external
				new (exbuf) FN{std::move(*pfn)};

				// destruct original
				destruct(buf);

				// assign external buffer to us
				reinterpret_cast<FN*&>(buf) = reinterpret_cast<FN*>(exbuf);
			}
		};
	}




	// vtable
	namespace detail
	{
		template <size_t BS, typename FN, typename R, typename... Params>
		auto mk_vtable(functor_storage_t s) -> functor_vtable_t<BS, R, Params...>*;

		template <size_t BS, functor_storage_t FS, typename FN, typename R, typename... Params>
		inline auto generate_vtable() -> functor_vtable_t<BS, R, Params...>*
		{
			using vtable_t = vtable_impl_t<BS, FS, FN, enable_SFO<BS, FN>(), R, Params...>;

			static auto _ = functor_vtable_t<BS, R, Params...>
			{
				&vtable_t::copy_construct,
				&vtable_t::destruct,
				&vtable_t::assign_into,
				&vtable_t::move_into,
				&vtable_t::external_size,
				&vtable_t::target,
				[]() -> std::type_info const& { return typeid(FN); },
				&vtable_t::call,
				&vtable_t::relocate,
				&mk_vtable<BS, FN, R, Params...>
			};

			return &_;
		}

		template <size_t BS, typename FN, typename R, typename... Params>
		inline auto mk_vtable(functor_storage_t s) -> functor_vtable_t<BS, R, Params...>*
		{
			switch (s)
			{
				case functor_storage_t::heap: return generate_vtable<BS, functor_storage_t::heap, FN, R, Params...>();
				case functor_storage_t::external: return generate_vtable<BS, functor_storage_t::external, FN, R, Params...>();
				case functor_storage_t::relative: return generate_vtable<BS, functor_storage_t::relative, FN, R, Params...>();
				default: return nullptr;
			}
		}
	}



	// dispatch
	namespace detail
	{
		template <size_t BS, typename R, typename... Params>
		using dispatch_fnptr_t = auto(*)(functor_buf_t<BS> const&, Params...) -> R;


		template <size_t BS, typename R, typename Params, typename Args, size_t I>                   struct dispatcher_t;
		template <size_t BS, typename R, typename Params, typename Args, size_t I, bool LT, bool GT> struct dispatcher_ii_t;
		template <size_t BS, typename R, typename Params, typename UParams, typename RParams>        struct dispatcher_iii_t;

		template <size_t BS, typename R, typename... Params, typename... UParams, typename... RParams>
		struct dispatcher_iii_t<BS, R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>
		{
			static auto call(dispatch_fnptr_t<BS, R, Params...>, functor_buf_t<BS> const&, UParams...) -> basic_generic_function_t<BS, functor_storage_t::heap, R(RParams...)>;
		};

		// superfluous arguments
		template <size_t BS, typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, true>
		{
			template <typename... More>
			static auto call(dispatch_fnptr_t<BS, R, Params...>, functor_buf_t<BS> const&, Params..., More&&...) -> R;
		};

		// partial-function
		template <size_t BS, typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, true, false>
			: dispatcher_iii_t<BS, R, std::tuple<Params...>,
				tuple_select_t<idxs_list_t<I>, std::tuple<Params...>>,
				tuple_select_t<idxs_range_t<I, sizeof...(Params)>, std::tuple<Params...>>>
		{
		};

		// correct arguments
		template <size_t BS, typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, false>
		{
			static auto call(dispatch_fnptr_t<BS, R, Params...>, functor_buf_t<BS> const&, Params...) -> R;
		};

		template <size_t BS, typename R, typename... Params, typename Args, size_t I>
		struct dispatcher_t<BS, R, std::tuple<Params...>, Args, I>
			: dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, (I < sizeof...(Params)), (sizeof...(Params) < I)>
		{
		};
	}

}


namespace atma {

	template <size_t BS, functor_storage_t FS, typename R, typename... Params>
	struct basic_generic_function_t<BS, FS, R(Params...)>
	{
		using this_type = basic_generic_function_t<BS, FS, R(Params...)>;

		basic_generic_function_t()
			: basic_generic_function_t{&empty_fn<R>}
		{}

		template <size_t RBS, functor_storage_t RS>
		basic_generic_function_t(basic_generic_function_t<RBS, RS, R(Params...)> const& rhs)
			: vtable_{rhs.vtable_->mk_vtable(FS)}
		{
			rhs.vtable_->assign_into(buf_, vtable_, rhs.buf_);
		}

		template <size_t RBS, functor_storage_t RS> // because otherwise FN version is more specialized
		basic_generic_function_t(basic_generic_function_t<RBS, RS, R(Params...)>& rhs)
			: basic_generic_function_t(std::as_const(rhs))
		{}

		template <size_t RBS, functor_storage_t RS>
		basic_generic_function_t(basic_generic_function_t<RBS, RS, R(Params...)>&& rhs)
			: vtable_{rhs.vtable_->mk_vtable<FS>()}
		{
			rhs.vtable_->move_into(buf_, vtable_, rhs.buf_);
		}

		template <typename FN, typename = std::enable_if_t<detail::result_matches_v<R, FN, Params...>>>
		basic_generic_function_t(FN&& fn)
			: vtable_{detail::generate_vtable<BS, FS, std::decay_t<FN>, R, Params...>()}
		{
			static_assert(FS == functor_storage_t::heap, "only heap-style functions can be initialized without external buffers");
			detail::constructing_t<BS, FS, std::decay_t<FN>>::construct(buf_, std::forward<FN>(fn));
		}

		template <typename FN, typename = std::enable_if_t<detail::result_matches_v<R, FN, Params...>>>
		basic_generic_function_t(FN&& fn, void* exbuf)
			: vtable_{detail::generate_vtable<BS, FS, std::decay_t<FN>, R, Params...>()}
		{
			detail::constructing_t<BS, FS, std::decay_t<FN>>::construct(buf_, exbuf, std::forward<FN>(fn));
		}

		template <size_t RBS, functor_storage_t RFS>
		auto operator = (basic_generic_function_t<RBS, RFS, R(Params...)> const& rhs) -> this_type&
		{
			if (this != &rhs)
			{
				vtable_ = rhs.vtable_->mk_vtable<FS>();
				rhs.vtable_->assign_into(buf_, vtable_, rhs.buf_);
			}

			return *this;
		}

		template <size_t RBS, functor_storage_t RFS>
		auto operator = (basic_generic_function_t<RBS, RFS, R(Params...)>&& rhs) -> this_type&
		{
			if (this != &rhs)
			{
				vtable_ = rhs.vtable_->mk_vtable<FS>();
				rhs.vtable_->move_into(buf_, vtable_, rhs.buf_);
			}

			return *this;
		}

		template <typename FN>
		auto operator = (FN&& fn) -> basic_generic_function_t&
		{
			vtable_->destruct(buf_);
			detail::constructing_t<BS, FS, std::decay_t<FN>>::construct(buf_, std::forward<FN>(fn));
			vtable_ = detail::generate_vtable<BS, FS, std::decay_t<FN>, R, Params...>();
			return *this;
		}

		auto operator = (std::nullptr_t) -> basic_generic_function_t&
		{
			return this->operator = (&empty_fn<R>);
		}

		operator bool() const
		{
			void* rawt = vtable_->target(const_cast<detail::functor_buf_t<BS>&>(buf_));
			ATMA_ASSERT(rawt != nullptr, "the null-case for function should be empty_fn, not nullptr");

			return *reinterpret_cast<decltype(&empty_fn<R>)*>(rawt) != &empty_fn<R>;
		}

		auto operator()(Params... args) const -> R
		{
			return vtable_->call(buf_, args...);
		}

		template <typename... Args>
		decltype(auto) operator ()(Args&&... args) const
		{
			return detail::dispatcher_t<BS, R, std::tuple<Params...>, std::tuple<Args...>, sizeof...(Args)>::call(vtable_->call, buf_, std::forward<Args>(args)...);
		}

		template <typename T>
		auto target() const -> T const*
		{
			void* rawt = vtable_->target(const_cast<detail::functor_buf_t<BS>&>(buf_));
			ATMA_ASSERT(rawt != nullptr, "the null-case for function should be empty_fn, not nullptr");

			if (*reinterpret_cast<decltype(&empty_fn<R>)*>(rawt) == &empty_fn<R>)
				return nullptr;

			std::type_info const& vti = vtable_->type_info();
			std::type_info const& tti = typeid(T);

			if (vti != tti)
				return nullptr;

			return (T const*)rawt;
		}

		auto swap(basic_generic_function_t& rhs) -> void
		{
			auto tmp = detail::functor_wrapper_t{};
			rhs.wrapper_.move_into(tmp);
			wrapper_.move_into(rhs.wrapper_);
			tmp.move_into(wrapper_);
		}

		auto relocate_external_buffer(void* exbuf) -> void
		{
			vtable_->relocate(buf_, exbuf);
		}

		auto external_buffer_size() const -> size_t
		{
			return vtable_->external_size();
		}

	protected:
		detail::functor_vtable_t<BS, R, Params...> const* vtable_;
		detail::functor_buf_t<BS> buf_;

		template <typename R2>
		static auto empty_fn(Params...) -> R2 { return *(R2*)nullptr; }
		template <>
		static auto empty_fn<void>(Params...) -> void {}

	private:
		template <size_t OBS, functor_storage_t OFS, typename OSIG>
		friend struct basic_generic_function_t;
	};

	namespace detail
	{
		template <size_t BS, typename R, typename... Params, typename Args, size_t I>
		template <typename... More>
		auto dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, true>::call(dispatch_fnptr_t<BS, R, Params...> fn, functor_buf_t<BS> const& buf, Params... params, More&&...) -> R
		{
			return fn(buf, params...);
		};

		template <size_t BS, typename R, typename... Params, typename Args, size_t I>
		inline auto dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, false>::call(dispatch_fnptr_t<BS, R, Params...> fn, functor_buf_t<BS> const& buf, Params... params) -> R
		{
			return fn(buf, params...);
		}

		template <size_t BS, typename R, typename... Params, typename... UParams, typename... RParams>
		auto dispatcher_iii_t<BS, R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>::call(dispatch_fnptr_t<BS, R, Params...> fn, functor_buf_t<BS> const& buf, UParams... params) -> basic_generic_function_t<BS, functor_storage_t::heap, R(RParams...)>
		{
			return basic_generic_function_t<BS, functor_storage_t::heap, R(RParams...)>{curry(fn, buf, params...)};
		}
	}




	//
	//  function_traits
	//  -----------------
	//    specialization for function_traits
	//
	template <size_t BS, functor_storage_t S, typename R, typename... Params>
	struct function_traits_override<basic_generic_function_t<BS, S, R(Params...)>>
		: function_traits<R(Params...)>
	{
	};


	//
	//  adapted_function_t
	//  --------------------
	//    given a callable, returns the basic_generic_function_t type that would be
	//    used to call it
	//
	namespace detail
	{
		template <typename R, typename Args> struct adapted_function_tx;

		template <typename R, typename... Args>
		struct adapted_function_tx<R, std::tuple<Args...>>
		{
			using type = function<R(Args...)>;
		};
	}

	template <typename FN>
	using adapted_function_t = typename detail::adapted_function_tx<
		typename function_traits<std::decay_t<FN>>::result_type,
		typename function_traits<std::decay_t<FN>>::tupled_args_type>::type;


	//
	//  functionize
	//  -------------
	//    wraps a callable in an basic_generic_function_t
	//
	template <typename FN>
	auto functionize(FN&& fn) -> adapted_function_t<FN>
	{
		return adapted_function_t<FN>{&std::forward<FN>(fn)};
	}


}
