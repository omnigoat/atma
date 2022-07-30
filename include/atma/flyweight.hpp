#pragma once

#include <memory>

import atma.intrusive_ptr;

namespace atma
{
	template <typename T>
	struct flyweight_t
	{
		using backend_ptr_t = std::conditional_t<std::is_convertible_v<T*, ref_counted*>, intrusive_ptr<T>, std::shared_ptr<T>>;

		flyweight_t();
		flyweight_t(flyweight_t const&) = default;
		flyweight_t(flyweight_t&&) = default;

		template <typename... Args>
		flyweight_t(Args&&...);

		auto operator = (flyweight_t const&) -> flyweight_t& = default;
		auto operator = (flyweight_t&&) -> flyweight_t& = default;

	protected:
		decltype(auto) backend() const { return *backend_; }
		decltype(auto) backend_ptr() const { return backend_; }

	private:
		backend_ptr_t backend_;
	};
}

namespace atma
{
	template <typename T>
	inline flyweight_t<T>::flyweight_t()
		: backend_{new T}
	{}

	template <typename T>
	template <typename... Args>
	inline flyweight_t<T>::flyweight_t(Args&&... args)
		: backend_{new T{std::forward<Args>(args)...}}
	{}
}

