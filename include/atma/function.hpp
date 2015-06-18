#pragma once

namespace atma
{
	// functor-buf
	namespace detail
	{
		// size of buffer to store functor. make this larger will allow for more
		// data to be stored in the "Small Function Optimization", but each instance
		// of function will take up more space.
		uint const bufsize_bytes = 32 - sizeof(void*) - sizeof(void*);

		struct functor_buf_t
		{
			char pad[bufsize_bytes];
		};
	}




	// vtable-impl
	namespace detail
	{
		template <typename FN>
		struct vtable_impl_t<FN, bool small_functor_optimization = sizeof(FN) <= bufsize_bytes>
		{
			static auto store(functor_buf_t& buf, FN const& fn) -> void
			{
				new (&buf) FN{fn};
			}

			static auto destruct(functor_buf_t& fn) -> void
			{
				reinterpret_cast<FN&>(fn).~FN();
			}

			static auto copy(functor_buf_t& dest, functor_buf_t const& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN const&>(src);
				new (&dest) FN{rhs};
			}

			static auto move(functor_buf_t& dest, functor_buf_t&& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN&&>(src);
				new (&dest) FN{std::move(rhs)};
			}

			template <typename R, typename... Args>
			static auto call(functor_buf_t const& buf, Args... args) -> R
			{
				auto& fn = reinterpret_cast<FN&>(const_cast<functor_buf_t&>(buf));
				return fn(std::forward<Args>(args)...);
			}
		};

		template <typename FN>
		struct vtable_impl_t<FN, false>
		{
			static auto store(functor_buf_t& buf, FN const& fn) -> void
			{
				reinterpret_cast<FN*&>(buf) = new FN(fn);
			}

			static auto move(functor_buf_t& dest, functor_buf_t&& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN*&>(src);
				auto&& lhs = reinterpret_cast<FN*&>(dest);
				lhs = new FN{std::move(*rhs)};
			}

			static auto copy(functor_buf_t& dest, functor_buf_t const& src) -> void
			{
				auto&& rhs = reinterpret_cast<FN* const&>(src);
				auto&& lhs = reinterpret_cast<FN*&>(dest);
				lhs = new FN{std::move(*rhs)};
			}

			static auto destruct(functor_buf_t& buf) -> void
			{
				auto&& tbuf = reinterpret_cast<FN*&>(buf);
				delete tbuf;
			}

			template <typename R, typename... Args>
			static auto call(functor_buf_t const& buf, Args... args) -> R
			{
				return (*reinterpret_cast<FN* const&>(buf))(std::forward<Args>(args)...);
			}
		};
	}




	// vtable
	namespace detail
	{
		template <typename R, typename... Params>
		struct functor_vtable_t
		{
			using destruct_fntype = auto(*)(detail::functor_buf_t&) -> void;
			using copy_fntype     = auto(*)(detail::functor_buf_t&, detail::functor_buf_t const&) -> void;
			using move_fntype     = auto(*)(detail::functor_buf_t&, detail::functor_buf_t&&) -> void;

			destruct_fntype destruct;
			copy_fntype     copy;
			move_fntype     move;
		};

		template <typename FN, typename R, typename... Params>
		auto generate_vtable() -> functor_vtable_t<R, Params...>*
		{
			static auto _ = functor_vtable_t<R, Params...>
			{
				&vtable_impl_t<FN>::destruct,
					&vtable_impl_t<FN>::copy,
					&vtable_impl_t<FN>::move,
			};

			return &_;
		}
	}




	// functor-wrapper
	namespace detail
	{
		template <typename R, typename... Params>
		struct functor_wrapper_t
		{
			functor_wrapper_t()
				: vtable_()
				, buf_{}
			{}

			template <typename FN>
			explicit functor_wrapper_t(FN&& fn)
				: vtable_{generate_vtable<FN, R, Params...>()}
				, buf_{}
			{
				vtable_impl_t<FN>::store(buf_, std::forward<FN>(fn));
			}

			functor_wrapper_t(functor_wrapper_t const& rhs)
				: vtable_{rhs.vtable_}
				, buf_{}
			{
				vtable_->copy(buf_, rhs.buf_);
			}

			functor_wrapper_t(functor_wrapper_t&& rhs)
				: vtable_{rhs.vtable_}
				, buf_{}
			{
				vtable_->move(buf_, std::move(rhs.buf_));
			}

			~functor_wrapper_t()
			{
				vtable_->destruct(buf_);
			}

			auto move_into(functor_wrapper_t& rhs) -> void
			{
				vtable_->move(rhs.buf_, std::move(buf_));
				rhs.vtable_ = vtable_;

				vtable_->destruct(buf_);
			}

			auto buf() const -> functor_buf_t const& { return buf_; }

		public:
			functor_vtable_t<R, Params...> const* vtable_;
			functor_buf_t buf_;
		};
	}




	// dispatch
	namespace detail
	{
		template <size_t, size_t, typename, typename> struct dispatcher_t;
		template <typename, typename, typename>        struct dispatcher_partial_t;

		template <typename R, typename... Params>
		using dispatch_fnptr_t = auto(*)(functor_buf_t const&, Params...) -> R;

		template <typename R, typename... Params, typename... RParams>
		struct dispatcher_partial_t<R, std::tuple<Params...>, std::tuple<RParams...>>
		{
			static auto call(dispatch_fnptr_t<R, Params..., RParams...>, functor_buf_t const&, Params...) -> function<R(RParams...)>;
		};

		// completed call
		template <size_t S, typename R, typename... Params>
		struct dispatcher_t<S, S, R, std::tuple<Params...>>
			: dispatcher_t<S, S - 1, R, std::tuple<Params...>>
		{
			using dispatcher_t<S, S - 1, R, std::tuple<Params...>>::call;

			static auto call(dispatch_fnptr_t<R, Params...>, functor_buf_t const&, Params...) -> R;
		};

		// partial call
		template <size_t PS, size_t S, typename R, typename... Params>
		struct dispatcher_t<PS, S, R, std::tuple<Params...>>
			: dispatcher_t<PS, S - 1, R, std::tuple<Params...>>
			, dispatcher_partial_t<R,
			atma::tuple_select_t<atma::idxs_list_t<S>, std::tuple<Params...>>,
			atma::tuple_select_t<atma::idxs_range_t<S, PS>, std::tuple<Params...>>
			>
		{
			using dispatcher_partial_t<R,
			atma::tuple_select_t<atma::idxs_list_t<S>, std::tuple<Params...>>,
			atma::tuple_select_t<atma::idxs_range_t<S, PS>, std::tuple<Params...>>
			>::call;
		};

		// terminate
		template <size_t PS, typename R, typename... Params>
		struct dispatcher_t<PS, 0, R, std::tuple<Params...>>
#if 0
			: dispatcher_partial_t<R,
			std::tuple<>,
			atma::tuple_select_t<atma::idxs_range_t<0, PS>, std::tuple<Params...>>
			>
#endif
		{};
	}




	// functorize
	namespace detail
	{
		template <typename T>
		auto functorize(T&& t) -> T
		{
			return t;
		}

		template <typename R, typename C, typename... Params>
		auto functorize(R(C::*fn)(Params...)) -> decltype(std::mem_fn(fn))
		{
			return std::mem_fn(fn);
		}

		template <typename R, typename C, typename... Params>
		auto functorize(R(C::*fn)(Params...) const) -> decltype(std::mem_fn(fn))
		{
			return std::mem_fn(fn);
		}
	}




	template <typename R, typename... Params>
	struct function<R(Params...)>
	{
		template <typename FN>
		explicit function(FN&& fn)
		{
			init_fn(detail::functorize(std::forward<FN>(fn)));
		}

		function(function&& rhs)
			: dispatch_{rhs.dispatch_}
			, wrapper_{std::move(rhs.wrapper_)}
		{
		}

		function(function const& rhs)
			: dispatch_{rhs.dispatch_}
			, wrapper_(rhs.wrapper_)
		{}

		auto operator()(Params... args) const -> R
		{
			return dispatch_(wrapper_.buf_, args...);
		}

		template <typename... Args>
		auto operator ()(Args&&... args) const
			-> decltype(detail::dispatcher_t<sizeof...(Params), sizeof...(Args), R, std::tuple<Params...>>::call(dispatch_, wrapper_.buf_, std::forward<Args>(args)...))
		{
			return detail::dispatcher_t<sizeof...(Params), sizeof...(Args), R, std::tuple<Params...>>::call(dispatch_, wrapper_.buf_, std::forward<Args>(args)...);
		}

		auto swap(function& rhs) -> void
		{
			auto tmp = detail::functor_wrapper_t{};
			rhs.wrapper_.move_into(tmp);
			wrapper_.move_into(rhs.wrapper_);
			tmp.move_into(wrapper_);
		}

	private:
		auto init_empty() -> void
		{
			using empty_fn = auto(*)(Params...) -> R;
			init_fn(empty_fn());
		}

		template <typename FN>
		auto init_fn(FN&& fn)
#if 1
			-> void
#else
			-> typename  std::enable_if<
			std::is_same<
			R,
			typename atma::function_traits<FN>::result_type
			>::value
			&&
			std::is_same<
			std::tuple<Params...>,
			typename atma::function_traits<FN>::tupled_args_type
			>::value
			>::type
#endif
		{
			dispatch_ = &detail::vtable_impl_t<FN>::template call<R, Params...>;
			new (&wrapper_) detail::functor_wrapper_t<R, Params...>{std::forward<FN>(fn)};
		}

	public:
		detail::dispatch_fnptr_t<R, Params...> dispatch_;
		detail::functor_wrapper_t<R, Params...> wrapper_;
	};
}
