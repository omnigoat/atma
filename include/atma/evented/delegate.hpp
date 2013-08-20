//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_EVENTED_DELEGATE_HPP
#define ATMA_EVENTED_DELEGATE_HPP
//=====================================================================
#include <atma/assert.hpp>
#include <atma/intrusive_ptr.hpp>
#include <atomic>
//=====================================================================
namespace atma {
namespace evented {
//=====================================================================
	

	//=====================================================================
	// delegate_t
	//=====================================================================
	template <typename R, typename... Params>
	struct delegate_t : atma::ref_counted
	{
		virtual ~delegate_t() {}
		virtual auto operator ()(Params&&...) -> R = 0;
	};


	//======================================================================
	// ...
	//======================================================================
	namespace detail
	{
		// fnptr
		template <typename R, typename... Params>
		struct fnptr_delegate_t : delegate_t<R, Params...>
		{
			fnptr_delegate_t(R(*fn)(Params...))
			 : fn_(fn)
			{
			}

			auto operator ()(Params&&... params) -> R override
			{
				return (*fn_)(std::forward<Params>(params)...);
			}

			R(*fn_)(Params...);
		};

		// fnptr
		template <typename R, typename T, typename... Params>
		struct bound_fnptr_delegate_t : delegate_t<R, Params...>
		{
			bound_fnptr_delegate_t(R(*fn)(Params...), T&& bindings)
			 : fn_(fn), bindings_(bindings)
			{
			}

			auto operator ()(Params&&... params) -> R override {
				return atma::xtm::apply_tuple_ex(fn_, bindings_, std::make_tuple(params));
			}

			R(*fn_)(Params...);
			T bindings_;
		};

		// memfnptr
		template <typename R, typename C, typename... Params>
		struct memfnptr_delegate_t : delegate_t<R, Params...>
		{
			memfnptr_delegate_t(R(C::*fn)(Params...), C* c)
			 : fn_(fn), c_(c)
			{
			}

			auto operator ()(Params&&... params) -> R override
			{
				return (c_->*fn_)(std::forward<Params>(params)...);
			}

			R(C::*fn_)(Params...);
			C* c_;
		};

		// callable
		template <typename F, typename R, typename... Params>
		struct callable_delegate_t : delegate_t<R, Params...>
		{
			callable_delegate_t(F&& fn)
			 : fn_(fn)
			{
			}

			auto operator ()(Params&&... params) -> R override
			{
				return fn_(params);
			}

			F fn_;
		};
	}
	

	template <typename R, typename... Params>
	inline auto make_delegate(R(*fn)(Params...)) -> intrusive_ptr<delegate_t<R, Params...>>
	{
		return intrusive_ptr<delegate_t<R, Params...>>(new detail::fnptr_delegate_t<R, Params...>(fn));
	}


	

//=====================================================================
} // namespace evented
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
