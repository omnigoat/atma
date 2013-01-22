//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_INTRUSIVE_PTR_HPP
#define ATMA_INTRUSIVE_PTR_HPP
//=====================================================================
#include <atma/assert.hpp>
//=====================================================================
namespace atma {
//=====================================================================
	
	struct ref_counted
	{
	protected:
		ref_counted() : ref_count_() {}
	private:
		unsigned int ref_count_;

		template <typename T> friend auto add_ref_count(T*) -> void;
		template <typename T> friend auto rm_ref_count(T*) -> void;
	};



	template <typename T>
	auto add_ref_count(T* t) -> void {
		ATMA_ASSERT(t);
		++static_cast<ref_counted*>(t)->ref_count_;
	}

	template <typename T>
	auto rm_ref_count(T* t) -> void {
		ATMA_ASSERT(t);
		if (--static_cast<ref_counted*>(t)->ref_count_ == 0) {
			delete t;
		}
	}



	template <typename T>
	struct intrusive_ptr
	{
		explicit intrusive_ptr(T* t, bool add_ref = true) 
		: px(t)
		{
			if (px && add_ref)
				add_ref_count(t);
		}

		intrusive_ptr(intrusive_ptr const& rhs)
		: px(rhs.px)
		{
			if (px)
				add_ref_count(px);
		}

		intrusive_ptr(intrusive_ptr&& rhs)
		: px()
		{
			std::swap(px, rhs.px);
		}

		~intrusive_ptr() {
			if (px) rm_ref_count(px);
		}

		auto operator = (intrusive_ptr<T> const& rhs) -> intrusive_ptr& {
			if (px)
				rm_ref_count(px);
			px = rhs.px;
			if (px)
				add_ref_count(px);
			return *this;
		}

		auto operator = (intrusive_ptr<T>&& rhs) -> intrusive_ptr& {
			if (px)
				rm_ref_count(px);
			px = rhs.px;
			rhs.px = nullptr;
		}

		auto operator * () const -> T* {
			return px;
		}
		
		auto operator -> () const -> T* {
			return px;
		}

		auto get() const -> T* {
			return px;
		}

	private:
		T* px;
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

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================
