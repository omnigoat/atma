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



	template <size_t, functor_storage_t, typename> struct basic_generic_function_t;
	
	template <size_t BufferSize, typename FN>
	using basic_function_t = basic_generic_function_t<BufferSize, functor_storage_t::heap, FN>;

	template <typename FN>
	using function = basic_generic_function_t<32, functor_storage_t::heap, FN>;



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
			using destruct_fntype  = auto(*)(functor_buf_t<BS>&) -> void;
			using assign_fntype    = auto(*)(functor_buf_t<BS>&, functor_vtable_t<BS, R, Params...> const*, functor_buf_t<BS> const&) -> void;
			using move_fntype      = auto(*)(functor_buf_t<BS>&, functor_vtable_t<BS, R, Params...> const*, functor_buf_t<BS>&&) -> void;
			using exsize_fntype    = auto(*)() -> size_t;
			using target_fntype    = auto(*)(functor_buf_t<BS>&) -> void*;
			using call_fntype      = auto(*)(functor_buf_t<BS> const&, Params...) -> R;
			using relocate_fntype  = auto(*)(functor_buf_t<BS>&, void*) -> void;
			using mk_vtable_fntype = auto(*)(functor_storage_t) -> functor_vtable_t*;

			destruct_fntype  destruct;
			assign_fntype    assign_into;
			move_fntype      move_into;
			exsize_fntype    external_size;
			target_fntype    target;
			call_fntype      call;
			relocate_fntype  relocate;
			mk_vtable_fntype mk_vtable;
		};

		template <size_t BS, typename FN, typename R, typename... Params>
		auto generate_vtable() -> functor_vtable_t<BS, R, Params...>*;
	}


	// constructing doesn't require a vtable
	namespace detail
	{
		template <size_t BS, typename FN, bool = enable_SFO<BS, FN>()>
		struct constructing_t
		{
			static auto construct(functor_buf_t<BS>& buf, FN const& fn) -> void
			{
				new (&buf) FN{ fn };
			}

			static auto construct(functor_buf_t<BS>& buf, void* exbuf, FN const& fn) -> void
			{
				new (&buf) FN{ fn };
			}

			static auto construct(functor_buf_t<BS>& buf, intptr offset, FN const& fn) -> void
			{
				new (&buf) FN{ fn };
			}
		};

		template <size_t BS, typename FN>
		struct constructing_t<BS, FN, false>
		{
			static auto construct(functor_buf_t<BS>& buf, FN const& fn) -> void
			{
				reinterpret_cast<FN*&>(buf) = new FN(fn);
			}

			static auto construct(functor_buf_t<BS>& buf, void* exbuf, FN const& fn) -> void
			{
				// buf becomes a pointer, pointing to exbuf, which houses fn
				FN*& p = reinterpret_cast<FN*&>(buf);
				p = reinterpret_cast<FN*>(exbuf);
				new (p) FN{ fn };
			}

			static auto construct(functor_buf_t<BS>& buf, intptr offset, FN const& fn) -> void
			{
				intptr& ip = reinterpret_cast<intptr&>(buf);
				ip = offset;
				FN* p = reinterpret_cast<FN*>(buf.buf + ip);
				new (p) FN{ fn };
			}
		};
	}


	// vtable-impl
	namespace detail
	{
		template <functor_storage_t, size_t BS, typename FN, bool, typename R, typename... Params>
		struct vtable_impl_t
		{
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
		struct vtable_impl_t<functor_storage_t::heap, BS, FN, false, R, Params...>
		{
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
				auto ptr = reinterpret_cast<FN*&>(buf);
				return ptr;
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
				auto ip = reinterpret_cast<intptr const&>(buf) & ~intptr(1);
				FN* pfn = reinterpret_cast<FN*&>(ip);

				// move-construct into new external
				new (exbuf) FN{std::move(*pfn)};

				// destruct original
				destruct(buf);

				// assign external buffer to us
				reinterpret_cast<FN*&>(buf) = reinterpret_cast<FN*>(exbuf);
				reinterpret_cast<intptr&>(buf) |= 1;
			}
		};

		template <size_t BS, typename FN, typename R, typename... Params>
		struct vtable_impl_t<functor_storage_t::external, BS, FN, false, R, Params...>
		{
			static auto destruct(functor_buf_t<BS>& buf) -> void
			{
				FN* p = reinterpret_cast<FN*&>(buf);
				p->~FN();
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
		struct vtable_impl_t<functor_storage_t::relative, BS, FN, false, R, Params...>
		{
			static auto destruct(functor_buf_t<BS>& buf) -> void
			{
				auto& ip = reinterpret_cast<intptr&>(buf);
				FN* p = reinterpret_cast<FN*>(buf.buf + ip);
				p->~FN();
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
		auto mk_vtable(functor_storage_t s)->functor_vtable_t<BS, R, Params...>*;

		template <size_t BS, typename FN, typename R, typename... Params>
		inline auto generate_vtable() -> functor_vtable_t<BS, R, Params...>*
		{
			using vtable_t = vtable_impl_t<functor_storage_t::heap, BS, FN, enable_SFO<BS, FN>(), R, Params...>;

			static auto _ = functor_vtable_t<BS, R, Params...>
			{
				&vtable_t::destruct,
				&vtable_t::assign_into,
				&vtable_t::move_into,
				&vtable_t::external_size,
				&vtable_t::target,
				&vtable_t::call,
				&vtable_t::relocate,
				&mk_vtable<BS, FN, R, Params...>
			};

			return &_;
		}

		template <size_t BS, typename FN, typename R, typename... Params>
		inline auto generate_vtable_exbuf() -> functor_vtable_t<BS, R, Params...>*
		{
			using vtable_t = vtable_impl_t<functor_storage_t::external, BS, FN, enable_SFO<BS, FN>(), R, Params...>;

			static auto _ = functor_vtable_t<BS, R, Params...>
			{
				&vtable_t::destruct,
				&vtable_t::assign_into,
				&vtable_t::move_into,
				&vtable_t::external_size,
				&vtable_t::target,
				&vtable_t::call,
				&vtable_t::relocate,
				&mk_vtable<BS, FN, R, Params...>
			};

			return &_;
		}

		template <size_t BS, typename FN, typename R, typename... Params>
		auto generate_vtable_rel() -> functor_vtable_t<BS, R, Params...>*
		{
			using vtable_t = vtable_impl_t<functor_storage_t::relative, BS, FN, enable_SFO<BS, FN>(), R, Params...>;

			static auto _ = functor_vtable_t<BS, R, Params...>
			{
				&vtable_t::destruct,
				&vtable_t::assign_into,
				&vtable_t::move_into,
				&vtable_t::external_size,
				&vtable_t::target,
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
			case functor_storage_t::heap: return generate_vtable<BS, FN, R, Params...>();
			case functor_storage_t::external: return generate_vtable_exbuf<BS, FN, R, Params...>();
			case functor_storage_t::relative: return generate_vtable_rel<BS, FN, R, Params...>();
			default: return nullptr;
			}
		}
	}




	// functor-wrapper
	namespace detail
	{
		template <size_t BS, typename R, typename... Params>
		struct functor_wrapper_t
		{
			template <typename FN>
			explicit functor_wrapper_t(FN&& fn)
				: vtable{generate_vtable<BS, std::decay_t<FN>, R, Params...>()}
				, buf{}
			{
				constructing_t<BS, std::decay_t<FN>>::construct(buf, fn);
			}

			template <typename FN>
			explicit functor_wrapper_t(void* exbuf, FN&& fn)
				: vtable{generate_vtable_exbuf<BS, std::decay_t<FN>, R, Params...>()}
				, buf{}
			{
				constructing_t<BS, std::decay_t<FN>>::construct(buf, exbuf, fn);
			}

			functor_wrapper_t(functor_wrapper_t const& rhs)
				: vtable{rhs.vtable}
				, buf{}
			{
				rhs.vtable->assign_into(buf, vtable, rhs.buf);
			}

			functor_wrapper_t(functor_wrapper_t&& rhs)
				: vtable{rhs.vtable}
				, buf{}
			{
				rhs.vtable->move_into(buf, vtable, std::move(rhs.buf));
			}

			~functor_wrapper_t()
			{
				vtable->destruct(buf);
			}

			auto operator = (functor_wrapper_t const& rhs) -> functor_wrapper_t&
			{
				rhs.vtable->assign_into(buf, vtable, rhs.buf);
				return *this;
			}

			auto operator = (functor_wrapper_t&& rhs) -> functor_wrapper_t&
			{
				rhs.vtable->move_into(buf, vtable, std::move(rhs.buf));
				return *this;
			}

			template <typename T>
			auto target() const -> T const*
			{
				return reinterpret_cast<T const*>(vtable->target(const_cast<detail::functor_buf_t<BS>&>(buf)));
			}

			auto move_into(functor_wrapper_t& rhs) -> void
			{
				
			}

			functor_vtable_t<BS, R, Params...> const* vtable;
			functor_buf_t<BS> buf;
		};
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

	template <size_t BS, functor_storage_t Storage, typename R, typename... Params>
	struct basic_generic_function_t<BS, Storage, R(Params...)>
	{
		using this_type = basic_generic_function_t<BS, Storage, R(Params...)>;

		basic_generic_function_t()
			: basic_generic_function_t{&empty_fn<R>}
		{}

		basic_generic_function_t(this_type const& rhs)
			: wrapper_(rhs.wrapper_)
		{}

		basic_generic_function_t(this_type&& rhs)
			: wrapper_{ std::move(rhs.wrapper_) }
		{}

		template <typename FN, typename = std::enable_if_t<detail::result_matches_v<R, FN, Params...>>>
		basic_generic_function_t(FN&& fn)
			: wrapper_{std::forward<FN>(fn)}
		{}

		template <typename FN, typename = std::enable_if_t<detail::result_matches_v<R, FN, Params...>>>
		basic_generic_function_t(FN&& fn, void* exbuf)
			: wrapper_{exbuf, std::forward<FN>(fn)}
		{}

		template <size_t RBS, functor_storage_t RStorage>
		auto operator = (basic_generic_function_t<RBS, RStorage, R(Params...)> const& rhs) -> this_type&
		{
			if (this != &rhs)
			{
				wrapper_.vtable = rhs.wrapper_.vtable->mk_vtable<Storage>();
				rhs.wrapper_.vtable->assign_into(wrapper_.buf, wrapper_.vtable, rhs.wrapper_.buf);
			}

			return *this;
		}

		auto operator = (basic_generic_function_t&& rhs) -> basic_generic_function_t&
		{
			if (this != &rhs)
			{
				reset();
				//dispatch_ = rhs.dispatch_;
				wrapper_  = std::move(rhs.wrapper_);
			}

			return *this;
		}

		template <typename FN>
		auto operator = (FN&& fn) -> basic_generic_function_t&
		{
			wrapper_.~functor_wrapper_t();
			init_fn(std::forward<FN>(fn));
			return *this;
		}

		auto operator = (std::nullptr_t) -> basic_generic_function_t&
		{
			wrapper_.~functor_wrapper_t();
			init_empty();
			return *this;
		}

		operator bool() const
		{
			return *target<R(*)(Params...)>() != empty_fn<R>;
		}

		auto operator()(Params... args) const -> R
		{
			//return dispatch_(wrapper_.buf, args...);
			//return wrapper_.vtable->call
			return wrapper_.vtable->call(wrapper_.buf, args...);
		}

		template <typename... Args>
		decltype(auto) operator ()(Args&&... args) const
		{
			return detail::dispatcher_t<BS, R, std::tuple<Params...>, std::tuple<Args...>, sizeof...(Args)>::call(wrapper_.vtable->call, wrapper_.buf, std::forward<Args>(args)...);
		}

		template <typename T>
		auto target() const -> T const*
		{
			return wrapper_.target<T>();
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
			wrapper_.vtable->relocate(wrapper_.buf, exbuf);
			//wrapper_.vtable->call = detail::vtable_impl_t<
		}

		auto external_buffer_size() const -> size_t
		{
			return wrapper_.vtable->external_size();
		}

	private:
		auto reset() -> void
		{
			//dispatch_ = nullptr;
			wrapper_.~functor_wrapper_t();
		}

		auto init_empty() -> void
		{
			init_fn(&empty_fn<R>);
		}

		template <typename FN>
		auto init_fn(FN&& fn) -> void
		{
			//dispatch_ = &detail::vtable_impl_t<detail::functor_storage_t::heap, BS, std::decay_t<FN>>::template call<R, Params...>;
			new (&wrapper_) detail::functor_wrapper_t<BS, R, Params...>{std::forward<FN>(fn)};
		}

		template <typename FN>
		auto init_fn(FN&& fn, void* exbuf) -> void
		{
			//dispatch_ = &detail::vtable_impl_t<detail::functor_storage_t::external, BS, std::decay_t<FN>>::template call<R, Params...>;
			new (&wrapper_) detail::functor_wrapper_t<BS, R, Params...>{exbuf, std::forward<FN>(fn)};
		}

	protected:
		//detail::dispatch_fnptr_t<BS, R, Params...>  dispatch_;
		detail::functor_wrapper_t<BS, R, Params...> wrapper_;

		template <typename R2>
		static auto empty_fn(Params...) -> R2 { return *(R2*)nullptr; }
		template <>
		static auto empty_fn<void>(Params...) -> void {}
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
