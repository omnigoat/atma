module;

#include <atma/functor.hpp>
#include <atomic>

export module atma.intrusive_ptr;


import atma.meta;

export namespace atma
{
	struct ref_counted;
	template <typename T> struct intrusive_ptr;
}


export namespace atma
{
	struct ref_counted
	{
		template <typename T>
		auto shared_from_this_as() -> intrusive_ptr<T>
		{
			return intrusive_ptr<T>(static_cast<T*>(this));
		}

	protected:
		ref_counted() = default;

		ref_counted(ref_counted const&) : ref_count_() {}

	private:
		mutable std::atomic_uint32_t ref_count_;

		template <typename, typename>
		friend struct ref_counted_traits;
	};

	template <typename T>
	struct ref_counted_of : ref_counted
	{
		auto shared_from_this() -> intrusive_ptr<T>
		{
			return intrusive_ptr<T>{static_cast<T*>(this)};
		}
	};
}

export namespace atma
{
	template <typename T, typename = std::void_t<>>
	struct ref_counted_traits
	{
		static auto add_ref(ref_counted const* t) -> void
		{
			if (t)
			{
				++t->ref_count_;
			}
		}

		static auto rm_ref(ref_counted const* t) -> void
		{
			if (t && --t->ref_count_ == 0)
			{
				delete t;
			}
		}
	};

	template <typename T>
	struct ref_counted_traits<T, std::void_t<decltype(std::declval<T>().ref_counted_add_ref()), decltype(std::declval<T>().ref_counted_rm_ref())>>
	{
		static auto add_ref(T const* t) -> void
		{
			if (t)
			{
				t->ref_counted_add_ref();
			}
		}

		static auto rm_ref(T const* t) -> void
		{
			if (t && t->ref_counted_rm_ref() == 0)
			{
				delete t;
			}
		}
	};
}


export namespace atma
{
	template <typename T>
	struct intrusive_ptr
	{
		using value_type = T;

		constexpr intrusive_ptr() noexcept = default;

		explicit constexpr intrusive_ptr(std::nullptr_t) noexcept
		{}

		explicit intrusive_ptr(T* t)
			: px(t)
		{
			ref_counted_traits<T>::add_ref(t);
		}

		template <typename Y>
		requires std::is_convertible_v<Y*, T*>
		explicit intrusive_ptr(Y* y)
			: px(y)
		{
			ref_counted_traits<T>::add_ref(y);
		}

		intrusive_ptr(intrusive_ptr const& rhs)
			: px(rhs.px)
		{
			ref_counted_traits<T>::add_ref(px);
		}

		template <typename Y>
		requires std::is_convertible_v<Y*, T*>
		intrusive_ptr(intrusive_ptr<Y> const& rhs)
			: px(rhs.px)
		{
			ref_counted_traits<T>::add_ref(px);
		}

		intrusive_ptr(intrusive_ptr&& rhs) noexcept
			: px(rhs.px)
		{
			rhs.px = nullptr;
		}

		template <typename Y>
		requires std::is_convertible_v<Y*, T*>
		intrusive_ptr(intrusive_ptr<Y>&& rhs) noexcept
			: px(rhs.px)
		{
			rhs.px = nullptr;
		}

		~intrusive_ptr()
		{
			ref_counted_traits<T>::rm_ref(px);
		}

		auto operator = (intrusive_ptr const& rhs) -> intrusive_ptr&
		{
			if (this != &rhs)
			{
				ref_counted_traits<T>::add_ref(rhs.px);
				ref_counted_traits<T>::rm_ref(px);
				px = rhs.px;
			}

			return *this;
		}

		template <typename Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>>>
		auto operator = (intrusive_ptr<Y> const& rhs) -> intrusive_ptr&
		{
			if (this != &rhs)
			{
				ref_counted_traits<T>::add_ref(rhs.px);
				ref_counted_traits<T>::rm_ref(px);
				px = rhs.px;
			}

			return *this;
		}

		operator bool() const
		{
			return px != nullptr;
		}

		auto operator ! () const -> bool
		{
			return px == nullptr;
		}

		auto operator * () const -> T&
		{
			return *px;
		}

		auto operator -> () const -> T*
		{
			return px;
		}

		auto get() const -> T*
		{
			return px;
		}

		auto reset() -> void
		{
			*this = intrusive_ptr{};
		}

		template <typename Y>
		auto cast_static() const -> intrusive_ptr<Y>
		{
			return intrusive_ptr<Y>(static_cast<Y*>(px));
		}

		template <typename Y>
		auto cast_dynamic() const -> intrusive_ptr<Y>
		{
			return intrusive_ptr<Y>(dynamic_cast<Y*>(px));
		}

		template <typename... Args>
		static auto make(Args&&... args)->intrusive_ptr<T>;

		static intrusive_ptr const null;

		friend void swap(intrusive_ptr& lhs, intrusive_ptr& rhs)
		{
			auto t = lhs.px;
			lhs.px = rhs.px;
			rhs.px = t;
		}

	private:
		T* px = nullptr;

		template <typename> friend struct intrusive_ptr;
	};

	template <typename T>
	inline intrusive_ptr<T> const intrusive_ptr<T>::null;
}


//
// non-member operators
//
export namespace atma
{
	template <typename T>
	inline auto operator == (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool
	{
		return lhs.get() == rhs.get();
	}

	template <typename T>
	inline auto operator == (intrusive_ptr<T> const& lhs, T const* rhs) -> bool
	{
		return lhs.get() == rhs;
	}

	template <typename T>
	inline auto operator == (T const* lhs, intrusive_ptr<T> const& rhs) -> bool
	{
		return lhs == rhs.get();
	}

	template <typename T>
	inline auto operator == (intrusive_ptr<T> const& lhs, std::nullptr_t) -> bool
	{
		return lhs.get() == nullptr;
	}

	template <typename T>
	inline auto operator == (std::nullptr_t, intrusive_ptr<T> const& rhs) -> bool
	{
		return nullptr = rhs.get();
	}

	template <typename T>
	inline auto operator != (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool
	{
		return lhs.get() != rhs.get();
	}

	template <typename T>
	inline auto operator < (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool
	{
		return lhs.get() < rhs.get();
	}

	template <typename Y, typename T>
	inline auto ptr_cast_static(atma::intrusive_ptr<T> const& ptr) -> atma::intrusive_ptr<Y>
	{
		return intrusive_ptr<Y>(static_cast<Y*>(ptr.get()));
	}
}



namespace atma::detail
{
	template <typename T, typename... Args>
	concept has_static_method_make = requires(Args&&... args)
	{
		{ T::make(std::forward<Args>(args)...) } -> std::same_as<intrusive_ptr<T>>;
	};
}

export namespace atma
{
	struct enable_intrusive_ptr_make
	{
		template <typename T, typename... Args>
		static auto make(Args&&... args)
		{
			return functor_list_t
			{
				[](auto&&... args) -> intrusive_ptr<T>
				requires detail::has_static_method_make<T, decltype(args)...>
				{
					return T::make(std::forward<Args>(args)...);
				},

				[](Args&&... args) -> intrusive_ptr<T>
				{
					return intrusive_ptr<T>(new T(std::forward<Args>(args)...));
				}

			}(std::forward<Args>(args)...);
		}
	};
}




//
// make
//
export namespace atma
{
	template <typename T>
	template <typename... Args>
	inline auto intrusive_ptr<T>::make(Args&&... args) -> intrusive_ptr<T>
	{
		return enable_intrusive_ptr_make::template make<T>(std::forward<Args>(args)...);
	}
}

export namespace atma
{
	template <typename T, typename... Args>
	inline auto make_intrusive(Args&&... args) -> intrusive_ptr<T>
	{
		return intrusive_ptr<T>::make(std::forward<Args>(args)...);
	}
}

//
// polymorphic_cast
//
export namespace atma
{
	template <typename R, typename T>
	requires std::is_convertible_v<R*, T*>
	inline auto polymorphic_cast(intrusive_ptr<T> const& x) -> intrusive_ptr<R>
	{
		return intrusive_ptr<R>(dynamic_cast<R*>(x.get()));
	}

	template <typename R, typename T>
	requires std::is_convertible_v<R*, T*>
	inline auto intrusive_ptr_cast(intrusive_ptr<T> const& x) -> intrusive_ptr<R>
	{
		return intrusive_ptr<R>(static_cast<R*>(x.get()));
	}
}
