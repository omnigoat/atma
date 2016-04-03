#pragma once

#include <tuple>


namespace atma
{
	//
	//  has_functor_operator_v
	//  ------------------------
	//    boolean. true if type T has an `operator ()`
	//
	namespace detail
	{
		// SFINAE test
		template <typename T>
		struct has_functor_operator_t
		{
		private:
			template <typename C> constexpr static bool test(decltype(&C::operator ())) { return true; }
			template <typename C> constexpr static bool test(...) { return false; }

		public:
			constexpr static bool value = test<T>(0);
		};
	}

	template <typename T>
	constexpr bool const has_functor_operator_v = detail::has_functor_operator_t<T>::value;


	//
	//  is_function_pointer_v
	//  -----------------------
	//    srsly, c++ std.
	//
	namespace detail
	{
		template <typename T>
		struct is_function_pointer_tx
			: std::false_type
		{};

		template <typename R, typename... Args>
		struct is_function_pointer_tx<R(*)(Args...)>
			: std::true_type
		{};
	}

	template <typename T>
	constexpr bool is_function_pointer_v = detail::is_function_pointer_tx<T>::value;


	//
	//  is_callable_v
	//  ---------------
	//    returns true for a function-pointer, a member-function-pointer, or a functor
	//
	template <typename T>
	constexpr bool is_callable_v =
		is_function_pointer_v<T> || std::is_member_function_pointer_v<T> || has_functor_operator_v<T>;






	//
	//  function_traits
	//  -----------------
	//    good ol' function traits
	//
	namespace detail
	{
		// functor
		template <typename T>
		struct function_traits_tx
			: function_traits_tx<decltype(&T::operator())>
		{
			// subtle interaction: a functor is *not* a member-function-pointer,
			// even though we derive the majority of our definition from the member-
			// function-pointer of its call-operator
			static bool const is_memfnptr = false;
		};

		// function-pointer
		template <typename R, typename... Args>
		struct function_traits_tx<R(*)(Args...)>
			: function_traits_tx<R(Args...)>
		{};

		// member-function-pointer
		template <typename C, typename R, typename... Args>
		struct function_traits_tx<R(C::*)(Args...) const>
			: function_traits_tx<R(Args...)>
		{
			constexpr static bool is_memfnptr = true;
			using class_type = C;
		};

		template <typename C, typename R, typename... Args>
		struct function_traits_tx<R(C::*)(Args...)>
			: function_traits_tx<R(Args...)>
		{
			constexpr static bool is_memfnptr = true;
			using class_type = C;
		};

		// function-type
		template <typename R, typename... Args>
		struct function_traits_tx<R(Args...)>
		{
			using result_type = R;
			using tupled_args_type = std::tuple<Args...>;

			template <size_t i>
			using arg_type = std::tuple_element_t<i, std::tuple<Args...>>;

			constexpr static size_t const arity = sizeof...(Args);
			constexpr static bool   const is_memfnptr = false;
			using class_type = void;
		};
	}

	template <typename T>
	struct function_traits_override
		: detail::function_traits_tx<T>
	{};

	template <typename F>
	using function_traits = function_traits_override<std::decay_t<F>>;
	

}
