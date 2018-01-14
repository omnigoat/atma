#pragma once

#include <atma/intrusive_ptr.hpp>


namespace atma
{
	template <typename T>
	struct flyweight_t
	{
		template <typename... Args>
		flyweight_t(Args&&... args)
			: backend_{new T{std::forward<Args>(args)...}}
		{}

		flyweight_t(flyweight_t const&) = default;
		flyweight_t(flyweight_t&&) = default;

		auto operator = (flyweight_t const&) -> flyweight_t& = default;
		auto operator = (flyweight_t&&) -> flyweight_t& = default;

	protected:
		auto backend() const -> T& { return *backend_; }
		auto weak_backend() const -> std::weak_ptr<T> { return backend_; }

	private:
		std::shared_ptr<T> backend_;
	};
}

