#pragma once

#include <atma/assert.hpp>
#include <atomic>
#include <type_traits>

namespace atma {
	
	struct ref_counted;
	template <typename T> struct intrusive_ptr;




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

		static auto add_ref(ref_counted const* t) -> void {
			t && ++t->ref_count_;
		}

		static auto rm_ref(ref_counted const* t) -> void {
			ATMA_ASSERT(!t || t->ref_count_ >= 0);

			if (t && --t->ref_count_ == 0) {
				delete t;
			}
		}

		template <typename> friend struct intrusive_ptr;
	};




	template <typename T>
	struct intrusive_ptr
	{
		intrusive_ptr()
		: px()
		{}

		explicit intrusive_ptr(std::nullptr_t)
		: px()
		{}

		explicit intrusive_ptr(T* t)
			: px(t)
		{
			ref_counted::add_ref(t);
		}

		template <typename Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>::value>>
		explicit intrusive_ptr(Y* t)
		: px(t)
		{
			ref_counted::add_ref(t);
		}

		intrusive_ptr(intrusive_ptr const& rhs)
		: px(rhs.px)
		{
			ref_counted::add_ref(px);
		}

		template <typename Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>::value>>
		intrusive_ptr(intrusive_ptr<Y> const& rhs)
		: px(rhs.px)
		{
			ref_counted::add_ref(px);
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
			ref_counted::rm_ref(px);
		}

		auto operator = (intrusive_ptr const& rhs) -> intrusive_ptr&
		{
			ATMA_ASSERT(this != &rhs);
			ref_counted::add_ref(rhs.px);
			ref_counted::rm_ref(px);
			px = rhs.px;
			return *this;
		}

		template <typename Y, typename = std::enable_if_t<std::is_convertible<Y*, T*>>>
		auto operator = (intrusive_ptr<Y> const& rhs) -> intrusive_ptr&
		{
			ref_counted::add_ref(rhs.px);
			ref_counted::rm_ref(px);
			px = rhs.px;
			return *this;
		}

		operator bool () {
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

		template <typename Y>
		auto cast_static() const -> intrusive_ptr<Y> {
			return intrusive_ptr<Y>(static_cast<Y*>(px));
		}

		template <typename Y>
		auto cast_dynamic() const -> intrusive_ptr<Y> {
			return intrusive_ptr<Y>(dynamic_cast<Y*>(px));
		}

	private:
		T* px;

		template <typename> friend struct intrusive_ptr;
	};




	template <typename T>
	inline auto operator == (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool {
		return lhs.get() == rhs.get();
	}

	template <typename T>
	inline auto operator != (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool {
		return lhs.get() != rhs.get();
	}

	template <typename T>
	inline auto operator < (intrusive_ptr<T> const& lhs, intrusive_ptr<T> const& rhs) -> bool {
		return lhs.get() < rhs.get();
	}

}

