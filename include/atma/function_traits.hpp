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
#if 0
		template <typename T>
		struct has_functor_operator_t
		{
		private:
			template <typename C> constexpr static bool test(decltype(&C::helloworld)) { return true; }
			template <typename C> constexpr static bool test(...) { return false; }

		public:
			constexpr static bool value = test<T>(0);
		};
#endif
		template <typename T>
		class has_functor_operator_t
		{
			typedef char one;
			typedef long two;

			template <typename C> static one test(decltype(&C::operator ()));
			template <typename C> static two test(...);

		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

	}

	template <typename T>
	constexpr bool const has_functor_operator_v = detail::has_functor_operator_t<T>::value;


	//
	//  is_callable_v
	//
	namespace detail
	{
		template <typename T>
		struct is_callable_t
			: has_functor_operator_t<T>
		{};

		template <typename R, typename... Args>
		struct is_callable_t<R(*)(Args...)> {
			constexpr static bool value = true;
		};

		template <typename R, typename C, typename... Args>
		struct is_callable_t<R(C::*)(Args...)> {
			constexpr static bool value = true;
		};

		template <typename R, typename C, typename... Args>
		struct is_callable_t<R(C::*)(Args...) const> {
			constexpr static bool value = true;
		};
	}

	template <typename T>
	constexpr bool const is_callable_v = detail::is_callable_t<T>::value;







	template <typename F>
	struct function_traits;




	// remove references
	template <typename F>
	struct function_traits<F&>
		: function_traits<F>
	{};

	template <typename F>
	struct function_traits<F&&>
		: function_traits<F>
	{};


	// presumed "callable"
	template <typename T>
	struct function_traits
		: function_traits<decltype(&T::operator())>
	{
		// subtle interaction: a functor is *not* a member-function-pointer,
		// even though we derive the majority of our definition from the member-
		// function-pointer of its call-operator
		static bool const is_memfnptr = false;
	};


	// function-pointer
	template <typename R, typename... Args>
	struct function_traits<R(*)(Args...)>
		: function_traits<R(Args...)>
	{};


	// member-function-pointer
	template <typename C, typename R, typename... Args>
	struct function_traits<R(C::*)(Args...) const>
		: function_traits<R(Args...)>
	{
		static bool   const is_memfnptr = true;
		using class_type = C;
	};

	template <typename C, typename R, typename... Args>
	struct function_traits<R(C::*)(Args...)>
		: function_traits<R(Args...)>
	{
		static bool   const is_memfnptr = true;
		using class_type = C;
	};


	template <typename R, typename... Args>
	struct function_traits<R(Args...)>
	{
		using result_type      = R;
		using tupled_args_type = std::tuple<Args...>;

		template <size_t i>
		using arg_type = typename std::tuple_element<i, std::tuple<Args...>>::type;

		constexpr static size_t const arity = sizeof...(Args);
		constexpr static bool   const is_memfnptr = false;
		using class_type = void;
	};




	// reimplement std::result_of
	template <typename F>
	struct result_of
	{
		using type = std::result_of_t<F>;
	};

	template <typename F>
	using result_of_t = typename result_of<F>::type;
}
