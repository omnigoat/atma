#pragma once

#include <atma/intrusive_ptr.hpp>


namespace atma
{
	template <typename T>
	struct flyweight_t
	{
		flyweight_t()
			: backend_{new T}
		{}

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
		using storage_ptr_t = std::conditional_t<std::is_convertible_v<T*, ref_counted*>, intrusive_ptr<T>, std::shared_ptr<T>>;

		storage_ptr_t backend_;
	};
}

