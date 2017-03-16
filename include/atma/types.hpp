#pragma once

#include <cstdint>
#include <type_traits>

using uchar  = unsigned char;
using ushort = unsigned short;
using uint   = unsigned int;
using ulong  = unsigned long;
using ullong = unsigned long long;

using byte = uchar;

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

using size_t = std::size_t;

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
	//  transfer_const_t
	//  ------------------
	//    takes T, and if const, transforms M to be const, otherwise just M
	//
	template <typename T, typename M>
	using transfer_const_t = std::conditional_t<std::is_const<T>::value, M const, M>;


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
}
