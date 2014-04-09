//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_COM_PTR_HPP
#define ATMA_COM_PTR_HPP
//=====================================================================
#include <atma/assert.hpp>
#include <atomic>
//=====================================================================
namespace atma {
//=====================================================================
	

	template <typename T>
	class com_ptr
	{
	public:
		com_ptr()
			: x_(nullptr)
		{}

		// raw constructor takes ownership
		com_ptr(T* x)
			: x_(x)
		{}

		com_ptr(com_ptr&& rhs)
			: x_(rhs.x_)
		{
			rhs.x_ = nullptr;
		}

		com_ptr(com_ptr const& rhs)
			: x_(rhs.x_)
		{
			if (x_) x_->AddRef();
		}

		~com_ptr()
		{
			if (x_) x_->Release();
		}

		auto operator -> () const -> T* {
			return x_;
		}

		auto operator = (com_ptr const& rhs) -> com_ptr&
		{
			if (x_) x_->Release();
			x_ = rhs.x_;
			if (x_) x_->AddRef();
			return *this;
		}

		auto assign() -> T** {
			reset();
			return &x_;
		}

		operator bool() {
			return x_ != nullptr;
		}

		auto get() const -> T* const& { return x_; }

		auto reset() -> void {
			if (x_) {
				x_->Release();
				x_ = nullptr;
			}
		}

		auto operator == (T* rhs) const -> bool
		{
			return x_ == rhs;
		}

	private:
		T* x_;
	};

	
	template <typename T>
	inline auto operator < (com_ptr<T> const& lhs, com_ptr<T> const& rhs) -> bool
	{
		return lhs.get() < rhs.get();
	}




	// make_com_ptr
	template <typename T>
	auto make_com_ptr(T* t) -> com_ptr<T>
	{
		return com_ptr<T>(t);
	}

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

