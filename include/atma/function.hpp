#pragma once

#include <atma/bind.hpp>
#include <atma/tuple.hpp>
#include <atma/function_composition.hpp>
#include <atma/assert.hpp>

#include <tuple>
#include <functional>


// where we store the functor if we can't SFO
namespace atma
{
	enum class functor_storage_t
	{
		heap,
		external,
		relative,
	};
}


// function declarations
namespace atma
{
	template <size_t, functor_storage_t, typename>
	struct basic_generic_function_t;
	
	template <size_t BufferSize, typename FN>
	using basic_function_t = basic_generic_function_t<BufferSize, functor_storage_t::heap, FN>;

	template <size_t BufferSize, typename FN>
	using basic_external_function_t = basic_generic_function_t<BufferSize, functor_storage_t::external, FN>;

	template <size_t BufferSize, typename FN>
	using basic_relative_function_t = basic_generic_function_t<BufferSize, functor_storage_t::relative, FN>;

	template <typename FN>
	using function = basic_function_t<32, FN>;

	template <typename FN>
	using external_function = basic_external_function_t<32, FN>;

	template <typename FN>
	using relative_function = basic_relative_function_t<32, FN>;
}


// functor-buf
namespace atma::detail
{
	// size of buffer to construct functor. make this larger will allow for more
	// data to be stored in the "Small Function Optimization", but each instance
	// of basic_generic_function_t will take up more space.
	//
	// the first 8 bytes are always reserved for a pointer to an external buffer,
	// no matter what style of functor-storage is used.
	struct functor_buf_t
	{
		template <typename FN = void>
		auto exbuf_address() -> FN* { return *(FN**)this; }

		template <typename FN = void>
		auto exbuf_address() const -> FN const* { return *(FN const**)this; }

		template <typename FN = void>
		auto relative_exbuf_address() -> FN* { return (FN*)((byte*)this + *(intptr*)this); }

		template <typename FN = void>
		auto relative_exbuf_address() const -> FN const* { return (FN const*)((byte const*)this + *(intptr*)this); }

		template <typename FN = void> auto functor_address() -> FN* { return (FN*)((byte const*)this + sizeof(void*)); }
		template <typename FN = void> auto functor_address() const -> FN const* { return (FN const*)((byte const*)this + sizeof(void*)); }

		auto assign_exbuf(void* p) {
			if (p != nullptr)
				*(void**)this = p;
		}

		auto assign_relative_exbuf(void* p) {
			if (p != nullptr)
				*(intptr*)this = (intptr)((byte*)p - (byte*)this);
		}
	};

	template <size_t Bytes>
	struct sized_functor_buf_t : functor_buf_t
	{
		byte buf[Bytes];

		sized_functor_buf_t()
			: buf{}
		{}

		static_assert(Bytes >= sizeof(void*), "functor_buf_t needs to be at least the size of a pointer");
	};

	template <size_t BS, typename FN>
	constexpr auto enable_SFO() -> bool
	{
		return sizeof(FN) + sizeof(void*) <= BS;
	}

	template <typename R, typename FN, typename... Params>
	constexpr bool result_matches_v = std::is_same_v<R, std::result_of_t<std::remove_reference_t<FN>(Params...)>>;
}


// vtable
namespace atma::detail
{
	template <typename R, typename... Params>
	struct functor_vtable_t
	{
		using copy_construct_fntype = auto(*)(functor_buf_t&, void*, void const*) -> void;
		using move_construct_fntype = auto(*)(functor_buf_t&, void*, void const*) -> void;
		using destruct_fntype       = auto(*)(functor_buf_t&) -> void;
		using assign_fntype         = auto(*)(functor_buf_t&, functor_vtable_t<R, Params...> const*&, functor_buf_t const&) -> void;
		using move_fntype           = auto(*)(functor_buf_t&, functor_vtable_t<R, Params...> const*&, functor_buf_t&&) -> void;
		using functor_store_fntype  = auto(*)() -> functor_storage_t;
		using buffersize_fntype     = auto(*)() -> size_t;
		using exsize_fntype         = auto(*)() -> size_t;
		using target_fntype         = auto(*)(functor_buf_t const&) -> void*;
		using type_info_fntype      = auto(*)() -> std::type_info const&;
		using call_fntype           = auto(*)(functor_buf_t const&, Params...) -> R;
		using relocate_fntype       = auto(*)(functor_buf_t&, void*) -> void;
		using mk_vtable_fntype      = auto(*)(size_t, functor_storage_t) -> functor_vtable_t<R, Params...> const*;

		copy_construct_fntype copy_construct;
		move_construct_fntype move_construct;
		destruct_fntype       destruct;
		assign_fntype         assign_into;
		move_fntype           move_into;
		functor_store_fntype  functor_storage;
		buffersize_fntype     buffer_size;
		exsize_fntype         external_size;
		target_fntype         target;
		type_info_fntype      type_info;
		call_fntype           call;
		relocate_fntype       relocate;
		mk_vtable_fntype      mk_vtable;
	};

	template <size_t BS, typename FN, typename R, typename... Params>
	inline auto mk_vtable(functor_storage_t) -> functor_vtable_t<R, Params...> const*;

	template <typename FN, typename R, typename... Params>
	inline auto mk_vtableR(size_t, functor_storage_t) -> functor_vtable_t<R, Params...> const*;


	template <size_t BS, functor_storage_t, typename FN, typename R, typename... Params>
	auto generate_vtable() -> functor_vtable_t<R, Params...> const*;
}


// constructing doesn't require a vtable
namespace atma::detail
{
	template <size_t BS, functor_storage_t FS, typename FN, bool = enable_SFO<BS, FN>()>
	struct constructing_t;

	template <size_t BS, functor_storage_t FS, typename FN>
	struct constructing_t<BS, FS, FN, true>
	{
		template <typename RFN>
		static auto construct(functor_buf_t& buf, void* exbuf, RFN&& fn) -> void
		{
			buf.assign_exbuf(exbuf);
			new (buf.functor_address()) FN{std::forward<RFN>(fn)};
		}
	};

	template <size_t BS, typename FN>
	struct constructing_t<BS, functor_storage_t::relative, FN, true>
	{
		template <typename RFN>
		static auto construct(functor_buf_t& buf, void* exbuf, RFN&& fn) -> void
		{
			buf.assign_relative_exbuf(exbuf);
			new (buf.functor_address()) FN{std::forward<RFN>(fn)};
		}
	};

	template <size_t BS, typename FN>
	struct constructing_t<BS, functor_storage_t::heap, FN, false>
	{
		template <typename RFN>
		static auto construct(functor_buf_t& buf, void* exbuf, RFN&& fn) -> void
		{
			ATMA_ASSERT(exbuf == nullptr);
			buf.assign_exbuf(new FN{std::forward<RFN>(fn)});
		}
	};

	template <size_t BS, typename FN>
	struct constructing_t<BS, functor_storage_t::external, FN, false>
	{
		template <typename RFN>
		static auto construct(functor_buf_t& buf, void* exbuf, RFN&& fn) -> void
		{
			buf.assign_exbuf(exbuf);
			new (buf.exbuf_address()) FN{std::forward<RFN>(fn)};
		}
	};

	template <size_t BS, typename FN>
	struct constructing_t<BS, functor_storage_t::relative, FN, false>
	{
		template <typename RFN>
		static auto construct(functor_buf_t& buf, void* exbuf, RFN&& fn) -> void
		{
			buf.assign_relative_exbuf(exbuf);
			new (buf.relative_exbuf_address()) FN{std::forward<RFN>(fn)};
		}
	};
}

// vtable-impl
namespace atma::detail
{
	template <size_t BS, functor_storage_t FS, typename FN, bool, typename R, typename... Params>
	struct vtable_impl_t
	{
		static auto copy_construct(functor_buf_t& buf, void* exbuf, void const* fnv) -> void
		{
			constructing_t<BS, FN>::construct(buf, exbuf, *((FN const*)fnv));
		}

		static auto move_construct(functor_buf_t& buf, void* exbuf, void const* fnv) -> void
		{
			constructing_t<BS, FN>::construct(buf, exbuf, std::move(*((FN const*)fnv)));
		}

		static auto destruct(functor_buf_t& buf) -> void
		{
			auto pfn = buf.functor_address<FN>();
			pfn->~FN();
		}

		static auto assign_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t const& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN const*)target(src);
			dvtable->copy_construct(dest, nullptr, sfn);
		}

		static auto move_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t&& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN*)target(src);
			dvtable->move_construct(dest, nullptr, sfn);
		}

		static auto external_size() -> size_t
		{
			// SFO has no external allocations
			return 0u;
		}

		static auto target(functor_buf_t const& buf) -> void*
		{
			return const_cast<functor_buf_t&>(buf).functor_address();
		}

		static auto type_info() -> std::type_info const&
		{
			return typeid(FN);
		}

		static auto call(functor_buf_t const& buf, Params... args) -> R
		{
			auto fn = buf.functor_address<FN>();
			return std::invoke(*fn, std::forward<Params>(args)...);
		}

		static auto relocate(functor_buf_t& buf, void*) -> void
		{
			// no relocation required for SFO
		}
	};

	template <size_t BS, typename FN, typename R, typename... Params>
	struct vtable_impl_t<BS, functor_storage_t::heap, FN, false, R, Params...>
	{
		static auto copy_construct(functor_buf_t& buf, void*, void const* fnv) -> void
		{
			auto fn = reinterpret_cast<FN const*>(fnv);
			reinterpret_cast<FN*&>(buf) = new FN{*fn};
		}

		static auto move_construct(functor_buf_t& buf, void*, void const* fnv) -> void
		{
			auto fn = reinterpret_cast<FN const*>(fnv);
			reinterpret_cast<FN*&>(buf) = new FN{std::move(*fn)};
		}

		static auto destruct(functor_buf_t& buf) -> void
		{
			FN*& p = reinterpret_cast<FN*&>(buf);
			delete p;
		}

		static auto assign_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t const& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN const*)target(src);
			dvtable->copy_construct(dest, nullptr, sfn);
		}

		static auto move_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t&& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN*)target(src);
			dvtable->move_construct(dest, nullptr, sfn);
		}

		static auto external_size() -> size_t
		{
			return sizeof(FN);
		}

		static auto target(functor_buf_t const& buf) -> void*
		{
			return const_cast<functor_buf_t&>(buf).exbuf_address();
		}

		static auto call(functor_buf_t const& buf, Params... args) -> R
		{
			auto fn = buf.exbuf_address<FN>();
			return std::invoke(*fn, std::forward<Params>(args)...);
		}

		static auto relocate(functor_buf_t& buf, void* exbuf) -> void
		{
			// SERIOUSLY, todo
		}
	};

	template <size_t BS, typename FN, typename R, typename... Params>
	struct vtable_impl_t<BS, functor_storage_t::external, FN, false, R, Params...>
	{
		static auto copy_construct(functor_buf_t& buf, void*, void const* fnv) -> void
		{
			auto fn = reinterpret_cast<FN const*>(fnv);
			new (target(buf)) FN{*fn};
		}

		static auto move_construct(functor_buf_t& buf, void*, void const* fnv) -> void
		{
			auto fn = reinterpret_cast<FN const*>(fnv);
			new (target(buf)) FN{std::move(*fn)};
		}

		static auto destruct(functor_buf_t& buf) -> void
		{
			FN* p = reinterpret_cast<FN*&>(buf);
			p->~FN();
		}

		static auto assign_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t const& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN const*)target(src);
			dvtable->copy_construct(dest, nullptr, sfn);
		}

		static auto move_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t&& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN*)target(src);
			dvtable->move_construct(dest, nullptr, sfn);
		}

		static auto external_size() -> size_t
		{
			return sizeof(FN);
		}

		static auto target(functor_buf_t const& buf) -> void*
		{
			return const_cast<functor_buf_t&>(buf).exbuf_address();
		}

		static auto call(functor_buf_t const& buf, Params... args) -> R
		{
			auto fn = buf.exbuf_address<FN>();
			return std::invoke(*fn, std::forward<Params>(args)...);
		}

		static auto relocate(functor_buf_t& buf, void* exbuf) -> void
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
		static auto copy_construct(functor_buf_t& buf, void*, void const* fnv) -> void
		{
			auto fn = reinterpret_cast<FN const*>(fnv);
			new (target(buf)) FN{*fn};
		}

		static auto move_construct(functor_buf_t& buf, void*, void const* fnv) -> void
		{
			auto fn = reinterpret_cast<FN const*>(fnv);
			new (target(buf)) FN{std::move(*fn)};
		}

		static auto destruct(functor_buf_t& buf) -> void
		{
			((FN*)target(buf))->~FN();
		}

		static auto assign_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t const& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN const*)target(src);
			dvtable->copy_construct(dest, nullptr, sfn);
		}

		static auto move_into(functor_buf_t& dest, functor_vtable_t<R, Params...> const*& dvtable, functor_buf_t&& src) -> void
		{
			dvtable->destruct(dest);
			dvtable = mk_vtableR<FN, R, Params...>(dvtable->buffer_size(), dvtable->functor_storage());
			auto sfn = (FN const*)target(src);
			dvtable->move_construct(dest, nullptr, sfn);
		}

		static auto external_size() -> size_t
		{
			return sizeof(FN);
		}

		static auto target(functor_buf_t const& buf) -> void*
		{
			return const_cast<functor_buf_t&>(buf).relative_exbuf_address();
		}

		static auto call(functor_buf_t const& buf, Params... args) -> R
		{
			auto fn = buf.relative_exbuf_address<FN>();
			return std::invoke(*fn, std::forward<Params>(args)...);
		}

		static auto relocate(functor_buf_t& buf, void* exbuf) -> void
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




// generate-vtable
namespace atma::detail
{
	template <typename FN, typename R, typename... Params>
	constexpr inline auto mk_vtable_impl(size_t c, size_t bs, functor_storage_t fs)
	{
		return (c == bs)
			? generate_vtable<c, fs, FN, R, Params...>()
			: mk_vtable_impl<FN, R, Params...>(c + 1, bs, fs);
	}

	template <size_t BS, typename FN, typename R, typename... Params>
	inline auto mk_vtable(functor_storage_t fs) -> functor_vtable_t<R, Params...> const*
	{
		switch (fs)
		{
			case functor_storage_t::heap:     return generate_vtable<BS, functor_storage_t::heap, FN, R, Params...>();
			case functor_storage_t::external: return generate_vtable<BS, functor_storage_t::external, FN, R, Params...>();
			case functor_storage_t::relative: return generate_vtable<BS, functor_storage_t::relative, FN, R, Params...>();
			default: return nullptr;
		}
	}

	template <typename FN, typename R, typename... Params>
	inline auto mk_vtableR(size_t size, functor_storage_t fs) -> functor_vtable_t<R, Params...> const*
	{
#define gentable(n) \
switch (fs)\
{\
			case functor_storage_t::heap:     return generate_vtable<n, functor_storage_t::heap, FN, R, Params...>();\
			case functor_storage_t::external: return generate_vtable<n, functor_storage_t::external, FN, R, Params...>();\
			case functor_storage_t::relative: return generate_vtable<n, functor_storage_t::relative, FN, R, Params...>();\
			default: return nullptr;\
}

		switch (size)
		{
			case  8: gentable( 8);
			case 16: gentable(16);
			case 24: gentable(24);
			case 32: gentable(32);
			case 40: gentable(40);
			case 48: gentable(48);
			case 56: gentable(56);
			case 64: gentable(64);
			default: ATMA_HALT("undefined mapping of buffer-size");
		}

#undef gentable
		return nullptr;
	}

	template <size_t BS, functor_storage_t FS, typename FN, typename R, typename... Params>
	inline auto generate_vtable() -> functor_vtable_t<R, Params...> const*
	{
		using vtable_t = vtable_impl_t<BS, FS, FN, enable_SFO<BS, FN>(), R, Params...>;

		static auto const _ = functor_vtable_t<R, Params...>
		{
			[](functor_buf_t& buf, void* exbuf, void const* fnv) { constructing_t<BS, FS, FN>::construct(buf, exbuf, *((FN const*)fnv)); },
			[](functor_buf_t& buf, void* exbuf, void const* fnv) { constructing_t<BS, FS, FN>::construct(buf, exbuf, std::move(*((FN const*)fnv))); },
			&vtable_t::destruct,
			&vtable_t::assign_into,
			&vtable_t::move_into,
			[] { return FS; },
			[] { return BS; },
			&vtable_t::external_size,
			&vtable_t::target,
			[]() -> std::type_info const& { return typeid(FN); },
			&vtable_t::call,
			&vtable_t::relocate,
			&mk_vtableR<FN, R, Params...>,
		};

		return &_;
	}
}



// dispatch
namespace atma::detail
{
	template <size_t BS, typename R, typename... Params>
	using dispatch_fnptr_t = auto(*)(functor_buf_t const&, Params...) -> R;

	template <size_t BS, typename R, typename Params, typename Args, size_t I>                   struct dispatcher_t;
	template <size_t BS, typename R, typename Params, typename Args, size_t I, bool LT, bool GT> struct dispatcher_ii_t;
	template <size_t BS, typename R, typename Params, typename UParams, typename RParams>        struct dispatcher_iii_t;

	// superfluous arguments
	template <size_t BS, typename R, typename... Params, typename Args, size_t I>
	struct dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, true>
	{
		template <typename... More>
		static auto call(dispatch_fnptr_t<BS, R, Params...>, functor_buf_t const&, Params..., More&&...) -> R;
	};

	// partial-function
	template <size_t BS, typename R, typename... Params, typename... UParams, typename... RParams>
	struct dispatcher_iii_t<BS, R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>
	{
		static auto call(dispatch_fnptr_t<BS, R, Params...>, functor_buf_t const&, UParams...) -> basic_generic_function_t<BS, functor_storage_t::heap, R(RParams...)>;
	};

	template <size_t BS, typename R, typename... Params, typename Args, size_t I>
	struct dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, true, false>
		: dispatcher_iii_t<BS, R, std::tuple<Params...>,
			tuple_select_t<idxs_list_t<I>, std::tuple<Params...>>,
			tuple_select_t<idxs_range_t<I, sizeof...(Params)>, std::tuple<Params...>>>
	{};

	// correct arguments
	template <size_t BS, typename R, typename... Params, typename Args, size_t I>
	struct dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, false>
	{
		static auto call(dispatch_fnptr_t<BS, R, Params...>, functor_buf_t const&, Params...) -> R;
	};

	// choose one of the three specializations
	template <size_t BS, typename R, typename... Params, typename Args, size_t I>
	struct dispatcher_t<BS, R, std::tuple<Params...>, Args, I>
		: dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, (I < sizeof...(Params)), (sizeof...(Params) < I)>
	{};
}


namespace atma
{
	template <size_t BS, typename FN>
	struct base_function_t;

	template <size_t BS, typename R, typename... Params>
	struct base_function_t<BS, R(Params...)>
	{
		~base_function_t()
		{
			vtable_->destruct(buf_);
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

		auto external_buffer_size() const -> size_t
		{
			return vtable_->external_size();
		}

		operator bool() const
		{
			void* rawt = vtable_->target(const_cast<detail::functor_buf_t&>((detail::functor_buf_t&)buf_));
			ATMA_ASSERT(rawt != nullptr, "the null-case for function should be empty_fn, not nullptr");

			return *reinterpret_cast<decltype(&empty_fn<R>)*>(rawt) != &empty_fn<R>;
		}

		template <typename T>
		auto target() const -> T const*
		{
			void* rawt = vtable_->target(const_cast<detail::functor_buf_t&>((detail::functor_buf_t&)buf_));
			ATMA_ASSERT(rawt != nullptr, "the null-case for function should be empty_fn, not nullptr");

			if (*reinterpret_cast<decltype(&empty_fn<R>)*>(rawt) == &empty_fn<R>)
				return nullptr;

			std::type_info const& vti = vtable_->type_info();
			std::type_info const& tti = typeid(T);

			if (vti != tti)
				return nullptr;

			return (T const*)rawt;
		}

	protected:
		base_function_t(detail::functor_vtable_t<R, Params...> const* vtable)
			: vtable_{vtable}
		{}

	protected:
		detail::sized_functor_buf_t<BS> buf_;
		detail::functor_vtable_t<R, Params...> const* vtable_;

		template <typename R2>
		static auto empty_fn(Params...) -> R2 { return *(R2*)nullptr; }
		template <>
		static auto empty_fn<void>(Params...) -> void {}

		template <size_t OBS, functor_storage_t OFS, typename OSIG>
		friend struct basic_generic_function_t;
	};
}

// basic_generic_function_t
namespace atma
{
	template <size_t BS, functor_storage_t FS, typename R, typename... Params>
	struct basic_generic_function_t<BS, FS, R(Params...)>
		: base_function_t<BS, R(Params...)>
	{
		using this_type = basic_generic_function_t<BS, FS, R(Params...)>;

		basic_generic_function_t()
			: basic_generic_function_t{&empty_fn<R>}
		{}

		basic_generic_function_t(basic_generic_function_t const& rhs)
			: base_function_t{rhs.vtable_->mk_vtable(BS, FS)}
		{
			rhs.vtable_->assign_into(buf_, vtable_, rhs.buf_);
		}

		basic_generic_function_t(basic_generic_function_t&& rhs)
			: base_function_t{rhs.vtable_->mk_vtable(BS, FS)}
		{
			rhs.vtable_->move_into(buf_, vtable_, std::move(rhs.buf_));
		}

		template <size_t RBS, functor_storage_t RS>
		basic_generic_function_t(basic_generic_function_t<RBS, RS, R(Params...)> const& rhs)
			: base_function_t{rhs.vtable_->mk_vtable(BS, FS)}
		{
			rhs.vtable_->assign_into(buf_, vtable_, rhs.buf_);
		}

		template <size_t RBS, functor_storage_t RS>
		basic_generic_function_t(basic_generic_function_t<RBS, RS, R(Params...)>&& rhs)
			: base_function_t{rhs.vtable_->mk_vtable(BS, FS)}
		{
			rhs.vtable_->move_into(buf_, vtable_, std::move(rhs.buf_));
		}

		template <size_t RBS, functor_storage_t RS>
		basic_generic_function_t(basic_generic_function_t<RBS, RS, R(Params...)>& rhs)
			: basic_generic_function_t{std::as_const(rhs)}
		{}

		template <size_t RBS, functor_storage_t RS>
		basic_generic_function_t(basic_generic_function_t<RBS, RS, R(Params...)> const&& rhs)
			: basic_generic_function_t{(this_type const&)rhs}
		{}

		template <typename FN, typename = std::enable_if_t<detail::result_matches_v<R, FN, Params...>>>
		basic_generic_function_t(FN&& fn)
			: base_function_t{detail::generate_vtable<BS, FS, std::decay_t<FN>, R, Params...>()}
		{
			static_assert(FS == functor_storage_t::heap, "only heap-style functions can be initialized without external buffers");
			detail::constructing_t<BS, FS, std::decay_t<FN>>::construct(buf_, nullptr, std::forward<FN>(fn));
		}

		template <size_t RBS>
		basic_generic_function_t(base_function_t<RBS, R(Params...)> const& rhs, void* exbuf)
			: basic_generic_function_t{&empty_fn<R>, exbuf}
		{
			rhs.vtable_->assign_into(buf_, vtable_, rhs.buf_);
		}

		template <typename FN,
			typename = std::enable_if_t<detail::result_matches_v<R, FN, Params...>>,
			typename = std::enable_if_t<!std::is_base_of<base_function_t<BS, R(Params...)>, std::decay_t<FN>>::value, basic_generic_function_t&>>
		basic_generic_function_t(FN&& fn, void* exbuf)
			: base_function_t{detail::generate_vtable<BS, FS, std::decay_t<FN>, R, Params...>()}
		{
			detail::constructing_t<BS, FS, std::decay_t<FN>>::construct(buf_, exbuf, std::forward<FN>(fn));
		}

		auto operator = (this_type const& rhs) -> this_type&
		{
			if (this != &rhs)
			{
				rhs.vtable_->assign_into(buf_, vtable_, rhs.buf_);
			}

			return *this;
		}

		auto operator = (this_type&& rhs) -> this_type&
		{
			if (this != &rhs)
			{
				rhs.vtable_->move_into(buf_, vtable_, std::move(rhs.buf_));
				vtable_ =  rhs.vtable_;
			}

			return *this;
		}

		template <size_t RBS>
		auto operator = (base_function_t<RBS, R(Params...)> const& rhs) -> this_type&
		{
			if (this != &rhs)
			{
				vtable_ = rhs.vtable_->mk_vtable<FS>();
				rhs.vtable_->assign_into(buf_, vtable_, rhs.buf_);
			}

			return *this;
		}

		template <size_t RBS>
		auto operator = (base_function_t<RBS, R(Params...)>&& rhs) -> this_type&
		{
			if (this != &rhs)
			{
				vtable_ = rhs.vtable_->mk_vtable<FS>();
				rhs.vtable_->move_into(buf_, vtable_, rhs.buf_);
			}

			return *this;
		}

		template <size_t RBS>
		auto operator = (base_function_t<RBS, R(Params...)>& rhs) -> this_type&
		{
			return this->operator = (std::as_const(rhs));
		}

		template <size_t RBS>
		auto operator = (base_function_t<RBS, R(Params...)> const&& rhs) -> this_type&
		{
			return this->operator = ((this_type const&)rhs);
		}

		template <typename FN>
		auto operator = (FN&& fn)
		-> std::enable_if_t<!std::is_base_of<base_function_t<BS, R(Params...)>, std::decay_t<FN>>::value, basic_generic_function_t&>
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

		auto swap(basic_generic_function_t& rhs) -> void
		{
			auto tmp = detail::functor_wrapper_t{};
			rhs.wrapper_.move_into(tmp);
			wrapper_.move_into(rhs.wrapper_);
			tmp.move_into(wrapper_);
		}

	private:
		template <size_t OBS, functor_storage_t OFS, typename OSIG>
		friend struct basic_generic_function_t;
	};
}



// dispatch implementation
namespace atma::detail
{
	template <size_t BS, typename R, typename... Params, typename Args, size_t I>
	template <typename... More>
	auto dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, true>::call(dispatch_fnptr_t<BS, R, Params...> fn, functor_buf_t const& buf, Params... params, More&&...) -> R
	{
		return fn(buf, params...);
	};

	template <size_t BS, typename R, typename... Params, typename Args, size_t I>
	inline auto dispatcher_ii_t<BS, R, std::tuple<Params...>, Args, I, false, false>::call(dispatch_fnptr_t<BS, R, Params...> fn, functor_buf_t const& buf, Params... params) -> R
	{
		return fn(buf, params...);
	}

	template <size_t BS, typename R, typename... Params, typename... UParams, typename... RParams>
	auto dispatcher_iii_t<BS, R, std::tuple<Params...>, std::tuple<UParams...>, std::tuple<RParams...>>::call(dispatch_fnptr_t<BS, R, Params...> fn, functor_buf_t const& buf, UParams... params) -> basic_generic_function_t<BS, functor_storage_t::heap, R(RParams...)>
	{
		return basic_generic_function_t<BS, functor_storage_t::heap, R(RParams...)>{curry(fn, buf, params...)};
	}
}


namespace atma
{
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
