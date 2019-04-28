#ifndef ATMA_ENABLE_MUIPTLE_SHARED_FROM_THIS_HPP
#define ATMA_ENABLE_MUIPTLE_SHARED_FROM_THIS_HPP
//=====================================================================
#include <memory>
//=====================================================================
namespace atma {
//=====================================================================
	
	struct enable_multiple_shared_from_this : std::enable_shared_from_this<enable_multiple_shared_from_this>
	{
		virtual ~enable_multiple_shared_from_this() {}

		template <typename T>
		auto shared_from_this() -> std::shared_ptr<T> { 
			return std::dynamic_pointer_cast<T>(::std::enable_shared_from_this<enable_multiple_shared_from_this>::shared_from_this());
		}

		template <typename T>
		auto shared_from_this() const -> std::shared_ptr<T const> {
			return std::dynamic_pointer_cast<T const>(::std::enable_shared_from_this<enable_multiple_shared_from_this>::shared_from_this());
		}

		template <typename T>
		auto weak_from_this() -> std::weak_ptr<T> {
			return std::dynamic_pointer_cast<T>(::std::enable_shared_from_this<enable_multiple_shared_from_this>::weak_from_this());
		}

		template <typename T>
		auto weak_from_this() const -> std::weak_ptr<T const> {
			return std::dynamic_pointer_cast<T const>(::std::enable_shared_from_this<enable_multiple_shared_from_this>::weak_from_this());
		}
	};

//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

