module;

#include <atma/functor.hpp>

#include <atomic>
#include <bit>
#include <memory>

export module atma.intrusive_ptr;


import atma.meta;
import atma.policies;




export namespace atma
{
	struct ref_counted;
	
	template <typename, typename...>
	struct intrusive_ptr;
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


namespace atma
{
	template <typename T>
	struct intrusive_ptr_traits
	{
		using pointer_type = T*;
		using reference_type = T&;

		template <typename Y>
		using rebind = intrusive_ptr_traits<Y>;

		static void assign_ptr(T*& t, T* x) { t = x; }
		static auto get_ptr(T* t) { return t; }

		template <typename, typename...>
		struct operations {};
	};
}

export namespace atma
{
	template <typename Deleter>
	struct use_deleter
	{
		using type = Deleter;
	};

	template <typename Traits>
	struct use_ref_counting_semantics
	{
		using type = Traits;
	};

	template <typename Traits>
	struct use_pointer_semantics
	{
		using type = Traits;
	};
}

namespace atma::detail
{
	template <typename Y, typename T>
	concept convertible_intrusive_ptr = std::convertible_to<
		typename intrusive_ptr_traits<Y>::pointer_type,
		typename intrusive_ptr_traits<T>::pointer_type>;

	// sigh, we do this because we need to inherit from T::operations
	template <typename T, typename... Policies>
	using operations_of_t = typename policies::retrieve_t<use_pointer_semantics,
		policies::default_<ref_counted_traits<T>>,
		Policies...>::template operations<T, Policies...>;
}

export namespace atma
{
	template <typename T, typename... Policies>
	struct intrusive_ptr
	{
		using deleter_type = policies::retrieve_t<use_deleter,
			policies::default_<std::default_delete<T>>,
			Policies...>;

		using refcount_traits = policies::retrieve_t<use_ref_counting_semantics,
			policies::default_<ref_counted_traits<T>>,
			Policies...>;

		using pointer_traits = policies::retrieve_t<use_pointer_semantics,
			policies::default_<intrusive_ptr_traits<T>>,
			Policies...>;

		using value_type = T;
		using pointer_type = typename pointer_traits::pointer_type;
		using reference_type = typename pointer_traits::reference_type;

		// shorthand for taking a convertible Y
		template <typename Y> using other_pointer_traits = typename pointer_traits::template rebind<Y>;
		template <typename Y> using other_pointer_type = typename other_pointer_traits<Y>::pointer_type;



		constexpr intrusive_ptr() noexcept = default;

		explicit constexpr intrusive_ptr(std::nullptr_t) noexcept
		{}

		explicit intrusive_ptr(pointer_type x)
			: px{x}
		{
			refcount_traits::add_ref(pointer_traits::get_ptr(px));
		}

		template <typename Y>
		requires detail::convertible_intrusive_ptr<Y, T>
		explicit intrusive_ptr(other_pointer_type<Y> y)
			: px{y}
		{
			refcount_traits::add_ref(pointer_traits::get_ptr(px));
		}

		intrusive_ptr(intrusive_ptr const& rhs)
			: px{rhs.px}
		{
			refcount_traits::add_ref(pointer_traits::get_ptr(px));
		}

		template <typename Y>
		requires detail::convertible_intrusive_ptr<Y, T>
		intrusive_ptr(intrusive_ptr<Y> const& rhs)
			: px{rhs.px}
		{
			refcount_traits::add_ref(pointer_traits::get_ptr(px));
		}

		intrusive_ptr(intrusive_ptr&& rhs) noexcept
			: px{std::move(rhs.px)}
		{
			pointer_traits::assign_ptr(rhs.px, nullptr);
		}

		template <typename Y>
		requires detail::convertible_intrusive_ptr<Y, T>
		intrusive_ptr(intrusive_ptr<Y>&& rhs) noexcept
			: px{std::move(rhs.px)}
		{
			other_pointer_traits<Y>::assign_ptr(rhs.px, nullptr);
		}

		~intrusive_ptr()
		{
			refcount_traits::rm_ref(pointer_traits::get_ptr(px));
		}

		auto operator = (intrusive_ptr const& rhs) -> intrusive_ptr&
		{
			if (this != &rhs)
			{
				refcount_traits::add_ref(pointer_traits::get_ptr(rhs.px));
				refcount_traits::rm_ref(pointer_traits::get_ptr(px));
				pointer_traits::assign_ptr(px, pointer_traits::get_ptr(rhs.px));
			}

			return *this;
		}

		template <typename Y>
		requires detail::convertible_intrusive_ptr<Y, T>
		auto operator = (intrusive_ptr<Y> const& rhs) -> intrusive_ptr&
		{
			if (this != &rhs)
			{
				refcount_traits::add_ref(other_pointer_traits<Y>::get_ptr(rhs.px));
				refcount_traits::rm_ref(pointer_traits::get_ptr(px));
				pointer_traits::assign_ptr(px, other_pointer_traits<Y>::get_ptr(rhs.px));
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

		auto operator * () const -> reference_type
		{
			return *pointer_traits::get_ptr(px);
		}

		auto operator -> () const -> pointer_type
		{
			return pointer_traits::get_ptr(px);
		}

		auto get() const -> pointer_type
		{
			return pointer_traits::get_ptr(px);
		}

		auto reset() -> void
		{
			*this = intrusive_ptr{};
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
		pointer_type px {};

		template <typename, typename...> friend struct intrusive_ptr;
	};

	template <typename T, typename... Policies>
	inline intrusive_ptr<T, Policies...> const intrusive_ptr<T, Policies...>::null;
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
		template <typename T, typename... Policies, typename... Args>
		static auto make(Args&&... args)
		{
			return functor_cascade_t
			{
				[](auto&&... args) -> intrusive_ptr<T, Policies...>
				requires detail::has_static_method_make<T, decltype(args)...>
				{
					return T::make(std::forward<Args>(args)...);
				},

				[](Args&&... args) -> intrusive_ptr<T, Policies...>
				{
					return intrusive_ptr<T, Policies...>(new T(std::forward<Args>(args)...));
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
	template <typename T, typename... Policies>
	template <typename... Args>
	inline auto intrusive_ptr<T, Policies...>::make(Args&&... args) -> intrusive_ptr<T>
	{
		return enable_intrusive_ptr_make::template make<T, Policies...>(std::forward<Args>(args)...);
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
// intrusive_ptr_cast
// polymorphic_cast
//
export namespace atma
{
	template <typename R, typename T>
	requires detail::convertible_intrusive_ptr<R, T>
	inline auto intrusive_ptr_cast(intrusive_ptr<T> const& x) -> intrusive_ptr<R>
	{
		return intrusive_ptr<R>{static_cast<R*>(x.get())};
	}

	template <typename R, typename T>
	requires std::is_convertible_v<R*, T*>
	inline auto polymorphic_cast(intrusive_ptr<T> const& x) -> intrusive_ptr<R>
	{
		return intrusive_ptr<R>(dynamic_cast<R*>(x.get()));
	}
}







namespace atma
{
	template <typename T>
	struct tagged_ptr_semantics
	{
		using pointer_type = T*;
		using reference_type = T&;
		using const_reference_type = T const&;

		template <typename Y>
		using rebind = tagged_ptr_semantics<Y>;


		inline static constexpr uintptr_t low_bits = std::bit_width(alignof(T));
		inline static constexpr uintptr_t low_bits_mask = alignof(T) - 1;


		static auto tag(pointer_type p) -> uintptr_t
		{
			return (uintptr_t)p & low_bits_mask;
		}

		static auto assign_ptr(pointer_type& p, pointer_type x) -> void
		{
			uintptr_t tag = take_tag(p);
			p = x;
			(uintptr_t)p |= tag;
		}

		static auto get_ptr(T* p) -> pointer_type
		{
			return static_cast<pointer_type>(static_cast<uintptr_t>(p) & ~low_bits_mask);
		}

		template <typename T, typename... P>
		struct operations
		{
			auto increment_tag()
			{
				auto p = static_cast<intrusive_ptr<T, P...>*>(this)->get();
				auto new_tag = (tag(p) + 1) & low_bits_mask;
				auto new_ptr = ((uintptr_t)this->get() & ~low_bits_mask) | new_tag;
			}
		};
	};

	template <typename T>
	using use_tagged_ptr_semantics = use_pointer_semantics<tagged_ptr_semantics<T>>;
}


