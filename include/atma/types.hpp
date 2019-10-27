#pragma once

#include <cstdint>
#include <type_traits>
#include <memory>

using uchar  = unsigned char;
using ushort = unsigned short;
using uint   = unsigned int;
using ulong  = unsigned long;
using ullong = unsigned long long;

using int8  = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using  intptr =  intptr_t;
using uintptr = uintptr_t;

using byte = uchar;

using size_t = std::size_t;


// void_t - msvc still seems to have issues
namespace atma
{
	template <class... Ts> struct make_void { using type = void; };
	template <class... Ts> using void_t = typename make_void<Ts...>::type;
}


#define ATMA_PP_CAT_II(a, b) a##b
#define ATMA_PP_CAT(a, b) ATMA_PP_CAT_II(a, b)

#define ATMA_SPLAT_FN_II(counter, pattern) int ATMA_PP_CAT(splat, counter)[] = {0, (pattern, void(), 0)...};
#define ATMA_SPLAT_FN(pattern) ATMA_SPLAT_FN_II(__COUNTER__, pattern)

#define ATMA_SFINAE_TEST_METHOD(name, oper) \
	template <typename T> \
	struct name##_t \
	{ \
	private: \
		template <typename C> constexpr static bool test(decltype(&C::oper)) { return true; } \
		template <typename C> constexpr static bool test(...) { return false; } \
		\
	public: \
		static constexpr bool value = test<T>(0); \
	}; \
	template <typename T> constexpr bool name##_v = name##_t<T>::value

namespace atma
{
	//
	//  actually_false
	//  ----------------
	//    for static_assert
	//
	template <typename...>
	constexpr bool actually_false = false;


	template <typename T>
	using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;


	//
	//  transfer_const_t
	//  ------------------
	//    takes T, and if const, transforms M to be const, otherwise just M
	//
	template <typename T, typename M>
	using transfer_const_t = std::conditional_t<std::is_const_v<T>, M const, M>;


	//
	//  has_functor_operator
	//  ----------------------
	//    boolean. true if type T has an `operator ()`
	//
	ATMA_SFINAE_TEST_METHOD(has_functor_operator, operator ());


	//
	//  is_function_pointer
	//  ---------------------
	//    srsly, c++ std.
	//
	template <typename T>
	struct is_function_pointer_t : std::conditional_t<
		std::is_pointer_v<T>,
		std::is_function<std::remove_pointer_t<T>>,
		std::false_type>
	{};

	template <typename T>
	constexpr static bool is_function_pointer_v = is_function_pointer_t<T>::value;


	//
	//  is_function_reference
	//  -----------------------
	//    srsly, c++ std.
	//
	template <typename T>
	struct is_function_reference_t : std::conditional_t<
		std::is_reference_v<T>,
		std::is_function<std::remove_reference_t<T>>,
		std::false_type>
	{};

	template <typename T>
	constexpr static bool is_function_reference_v = is_function_reference_t<T>::value;


	//
	//  is_callable_v
	//  ---------------
	//    returns true if T can be used like "T(args...)"
	//
	template <typename T>
	constexpr bool is_callable_v =
		is_function_pointer_v<T> ||
		is_function_reference_v<T> ||
		std::is_member_function_pointer_v<T> ||
		has_functor_operator_v<T>;

	//
	//  storage_type_t
	//  ----------------
	//    takes a type, and if it is an lvalue, keeps it as an lvalue,
	//    for any other type returns a non-reference type
	//
	template <typename T>
	using storage_type_t = std::conditional_t<std::is_rvalue_reference_v<T>, std::remove_reference_t<T>, T>;
}

//======================================================================
//  is_range_v
//======================================================================
namespace atma
{
	namespace detail
	{
		template <typename T, typename = std::void_t<>>
		struct is_range
		{
			static constexpr bool value = false;
		};

		template <typename T>
		struct is_range<T, std::void_t<
			decltype(std::begin(std::declval<T>())),
			decltype(std::end(std::declval<T>()))>>
		{
			static constexpr bool value = true;
		};
	}

	template <typename T>
	inline constexpr bool is_range_v = detail::is_range<T>::value;
}


//
//  value_type_of
//  ---------------
//    deducing value-type from R
//    
//     - tries remove-ref(*begin(R))
//     - tries R::value_type
//     - fails
//
namespace atma
{
	namespace detail
	{
		// value_type_of_ii
		template <typename R, typename = std::void_t<>>
		struct value_type_of_ii
		{
			struct nilx_ {};

			using type = nilx_;
		};

		template <typename R>
		struct value_type_of_ii<R, std::void_t<typename std::remove_reference_t<R>::value_type>>
		{
			using type = typename std::remove_reference_t<R>::value_type;
		};

		// value_type_of
		template <typename R, typename = std::void_t<>>
		struct value_type_of
		{
			using type = typename value_type_of_ii<R>::type;
		};

		template <typename R>
		struct value_type_of<R, std::void_t<decltype(*begin(std::declval<R&>()))>>
		{
			using type = std::remove_reference_t<decltype(*begin(std::declval<R&>()))>;
		};
	}

	template <typename R>
	using value_type_of_t = typename detail::value_type_of<R>::type;
}


//
//  allocator_type_of
//  -------------------
//    returns a "Range"'s allocator, or std::allocator<value-type> if possible
//
namespace atma
{
	template <typename R, typename = std::void_t<>>
	struct allocator_type_of
	{
		using type = std::allocator<value_type_of_t<R>>;
	};

	template <typename R>
	struct allocator_type_of<R, std::void_t<typename R::allocator_type>>
	{
		using type = typename R::allocator_type;
	};

	template <typename R>
	using allocator_type_of_t = typename allocator_type_of<std::remove_reference_t<R>>::type;
}

namespace atma
{
	template <typename T>
	using rmref_t = std::remove_reference_t<T>;

	template <typename T>
	using rm_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
}
