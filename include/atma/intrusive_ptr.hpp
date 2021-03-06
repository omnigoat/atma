#pragma once

#include <atma/assert.hpp>
#include <atomic>
#include <type_traits>

namespace atma
{
	struct ref_counted;
	template <typename T> struct intrusive_ptr;
}


namespace atma
{
	struct ref_counted
	{
		virtual ~ref_counted() {}

		template <typename T>
		auto shared_from_this() -> intrusive_ptr<T>
		{
			return intrusive_ptr<T>(static_cast<T*>(this));
		}

	protected:
		ref_counted() : ref_count_() {}
		ref_counted(ref_counted const&) : ref_count_() {}

	private:
		mutable std::atomic_uint32_t ref_count_;

		template <typename, typename> friend struct ref_counted_traits;
	};

	template <typename T>
	struct ref_counted_of : ref_counted
	{
		auto shared_from_this() -> intrusive_ptr<T>
		{
			return intrusive_ptr<T>{static_cast<T*>(this)};
		}

		template <typename Y>
		auto shared_from_this_as() -> intrusive_ptr<Y>
		{
			return intrusive_ptr<Y>(static_cast<Y*>(this));
		}
	};

	template <typename T, typename = std::void_t<>>
	struct ref_counted_traits
	{
		static auto add_ref(ref_counted const* t) -> void {
			t && ++t->ref_count_;
		}

		static auto rm_ref(ref_counted const* t) -> void {
			ATMA_ASSERT(!t || t->ref_count_ >= 0);

			if (t && --t->ref_count_ == 0) {
				delete t;
			}
		}
	};


	template <typename T>
	struct ref_counted_traits<T, std::void_t<decltype(&T::increment_refcount), decltype(&T::decrement_refcount)>>
	{
		static auto add_ref(T const* t) -> void
		{
			t && t->increment_refcount();
		}

		static auto rm_ref(T const* t) -> void
		{
			if (t && t->decrement_refcount() == 0)
			{
				delete t;
			}
		}
	};
}


namespace atma
{
	template <typename T>
	struct intrusive_ptr
	{
		using value_type = T;

		intrusive_ptr()
			: px()
		{}

		explicit intrusive_ptr(std::nullptr_t)
			: px()
		{}

		explicit intrusive_ptr(T* t)
			: px(t)
		{
			ref_counted_traits<T>::add_ref(t);
		}

		template <typename Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>::value>>
		explicit intrusive_ptr(Y* t)
			: px(t)
		{
			ref_counted_traits<T>::add_ref(t);
		}

		intrusive_ptr(intrusive_ptr const& rhs)
			: px(rhs.px)
		{
			ref_counted_traits<T>::add_ref(px);
		}

		template <typename Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>::value>>
		intrusive_ptr(intrusive_ptr<Y> const& rhs)
			: px(rhs.px)
		{
			ref_counted_traits<T>::add_ref(px);
		}
		
		intrusive_ptr(intrusive_ptr&& rhs)
			: px(rhs.px)
		{
			rhs.px = nullptr;
		}

		template <class Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>::value>>
		intrusive_ptr(intrusive_ptr<Y>&& rhs)
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
			ATMA_ASSERT(this != &rhs);
			ref_counted_traits<T>::add_ref(rhs.px);
			ref_counted_traits<T>::rm_ref(px);
			px = rhs.px;
			return *this;
		}

		template <typename Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>>>
		auto operator = (intrusive_ptr<Y> const& rhs) -> intrusive_ptr&
		{
			ref_counted_traits<T>::add_ref(rhs.px);
			ref_counted_traits<T>::rm_ref(px);
			px = rhs.px;
			return *this;
		}

		operator bool () const {
			return px != nullptr;
		}

		auto operator ! () const -> bool {
			return px == nullptr;
		}

		auto operator * () const -> T& {
			return *px;
		}
		
		auto operator -> () const -> T* {
			return px;
		}

		auto get() const -> T* {
			return px;
		}

		auto reset() -> void {
			*this = intrusive_ptr{};
		}

		template <typename Y>
		auto cast_static() const -> intrusive_ptr<Y> {
			return intrusive_ptr<Y>(static_cast<Y*>(px));
		}

		template <typename Y>
		auto cast_dynamic() const -> intrusive_ptr<Y> {
			return intrusive_ptr<Y>(dynamic_cast<Y*>(px));
		}

		template <typename... Args>
		static auto make(Args&&... args) -> intrusive_ptr<T>;

		static intrusive_ptr null;

		friend void swap(intrusive_ptr& lhs, intrusive_ptr& rhs)
		{
			using namespace std;
			swap(lhs.px, rhs.px);
		}

	private:
		T* px;

		template <typename> friend struct intrusive_ptr;
	};

	template <typename T>
	intrusive_ptr<T> intrusive_ptr<T>::null = intrusive_ptr<T>();


	template <typename T>
	inline auto operator == (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool {
		return lhs.get() == rhs.get();
	}

	template <typename T>
	inline auto operator == (intrusive_ptr<T> const& lhs, T const* rhs) -> bool {
		return lhs.get() == rhs;
	}

	template <typename T>
	inline auto operator == (T const* lhs, intrusive_ptr<T> const& rhs) -> bool {
		return lhs == rhs.get();
	}

	template <typename T>
	inline auto operator == (intrusive_ptr<T> const& lhs, std::nullptr_t) -> bool {
		return lhs.get() == nullptr;
	}

	template <typename T>
	inline auto operator == (std::nullptr_t, intrusive_ptr<T> const& rhs) -> bool {
		return nullptr = rhs.get();
	}

	template <typename T>
	inline auto operator != (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool {
		return lhs.get() != rhs.get();
	}

	template <typename T>
	inline auto operator < (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool {
		return lhs.get() < rhs.get();
	}


	template <typename Y, typename T>
	inline auto ptr_cast_static(atma::intrusive_ptr<T> const& ptr) -> atma::intrusive_ptr<Y>
	{
		return intrusive_ptr<Y>(static_cast<Y*>(ptr.get()));
	}

	struct enable_intrusive_ptr_make
	{
	private:
		template <typename T, typename A, typename = std::void_t<>>
		struct maker {
			template <typename... Args>
			static auto make(Args&&... args) -> T*
			{
				return new T(std::forward<Args>(args)...);
			}
		};

		template <typename T, typename... Args>
		struct maker<T, std::tuple<Args...>, std::void_t<decltype(T::make(std::declval<Args>()...))>>
		{
			template <typename... Args>
			static auto make(Args&&... args) -> T*
			{
				return T::make(std::forward<Args>(args)...);
			}
		};

	public:
		template <typename T, typename... Args>
		static auto make(Args&&... args) -> T*
		{
			return maker<T, std::tuple<Args...>>::make(std::forward<Args>(args)...);
		}
	};

	template <typename T>
	template <typename... Args>
	inline auto intrusive_ptr<T>::make(Args&&... args) -> intrusive_ptr<T>
	{
		return intrusive_ptr{enable_intrusive_ptr_make::template make<T>(std::forward<Args>(args)...)};
	}



	template <typename T, typename... Args>
	inline auto make_intrusive(Args&&... args) -> intrusive_ptr<T>
	{
		return intrusive_ptr<T>::make(std::forward<Args>(args)...);
	}
}

