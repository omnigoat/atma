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
	

	//=====================================================================
	// ref_counted
	// -------------
	//   this is a *non-virtual* base class.
	//=====================================================================
	struct ref_counted
	{
	protected:
		ref_counted() : ref_count_() {}

	private:
		unsigned int ref_count_;

		friend auto add_ref_count(ref_counted*) -> void;
		friend auto rm_ref_count(ref_counted*) -> void;
	};



	//=====================================================================
	// add/rm_ref_count
	// ------------------
	//   called by intrusive_ptr
	//=====================================================================
	inline auto add_ref_count(ref_counted* t) -> void {
		t && ++t->ref_count_;
	}

	inline auto rm_ref_count(ref_counted* t) -> void {
		ATMA_ASSERT(!t || t->ref_count_ >= 0);

		if (t && --t->ref_count_ == 0) {
			delete t;
		}
	}



	//=====================================================================
	// intrusive_ptr
	// ---------------
	//   atma's intrusive pointer implementation
	//=====================================================================
	template <typename T>
	struct intrusive_ptr
	{
		// constructors/destrectuor
		intrusive_ptr()
		: px()
		{
		}

		template <typename Y>
		explicit intrusive_ptr(Y* t) 
		: px(static_cast<T*>(t))
		{
			add_ref_count(t);
		}
		
		template <typename Y>
		intrusive_ptr(intrusive_ptr<Y> const& rhs)
		: px(static_cast<T*>(rhs.px))
		{
			add_ref_count(px);
		}

		template <typename Y>
		intrusive_ptr(intrusive_ptr<Y>&& rhs)
		: px(static_cast<T*>(rhs.px))
		{
			rhs.px = nullptr;
		}

		~intrusive_ptr() {
			rm_ref_count(px);
		}

		// operators
		template <typename Y>
		auto operator = (intrusive_ptr<Y> const& rhs) -> intrusive_ptr& {
			rm_ref_count(px);
			px = static_cast<T*>(rhs.px);
			add_ref_count(px);
			return *this;
		}

		template <typename Y>
		auto operator = (intrusive_ptr<Y>&& rhs) -> intrusive_ptr& {
			rm_ref_count(px);
			px = static_cast<T*>(rhs.px);
			rhs.px = nullptr;
			return *this;
		}

		template <typename Y>
		auto operator = (Y* y) -> intrusive_ptr& {
			rm_ref_count(px);
			px = static_cast<T*>(y);
			add_ref_count(px);
			return *this;
		}

		auto operator * () const -> T* {
			return px;
		}
		
		auto operator -> () const -> T* {
			return px;
		}

		// accessors
		auto get() const -> T* {
			return px;
		}

		template <typename Y>
		intrusive_ptr<Y> as() const {
			return intrusive_ptr<Y>(static_cast<Y*>(px));
		}

	private:
		T* px;

		template <typename Y>
		friend struct intrusive_ptr;
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
