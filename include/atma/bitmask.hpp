#pragma once

namespace atma
{
	template <typename T>
	struct bitmask_t
	{
		using storage_type = std::underlying_type_t<T>;

		bitmask_t()
			: mask_()
		{}

		bitmask_t(T x)
			: mask_((T)(1 << (storage_type)x))
		{}

		bitmask_t(std::initializer_list<T> const& xs)
			: mask_()
		{
			for (auto x : xs)
				(storage_type&)mask_ |= 1 << (storage_type)x;
		}

		auto operator & (T x) const -> bool {
			return ((storage_type&)mask_ & (1 << (storage_type)x)) != 0;
		}

		auto operator |= (T x) -> void {
			(storage_type&)mask_ |= (1 << (storage_type)x);
		}

		operator storage_type() const {
			return mask_;
		}

		static bitmask_t const none;

	private:
		T mask_;
	};

	template <typename T>
	bitmask_t<T> const bitmask_t<T>::none = bitmask_t<T>();

	template <typename T>
	inline auto operator | (bitmask_t<T> const& lhs, T rhs) -> bitmask_t<T>
	{
		auto tmp = lhs;
		tmp |= rhs;
		return tmp;
	}


#define ATMA_BITMASK_OR_OPERATOR(enum_typename) \
	inline auto operator | (enum_typename lhs, enum_typename rhs) -> ::atma::bitmask_t<enum_typename> { \
		return {lhs, rhs}; \
	}

#define ATMA_BITMASK(mask_typename, enum_typename) \
	using mask_typename = ::atma::bitmask_t<enum_typename>; \
	ATMA_BITMASK_OR_OPERATOR(enum_typename)

}