#ifndef ATMA_REF_HPP
#define ATMA_REF_HPP
//=====================================================================
namespace atma {
//=====================================================================

	//=====================================================================
	// allows people to pass references of things to algorithms/generics
	// that take their arguments by values.
	//=====================================================================
	template <typename T>
	class reference_wrapper
	{
		T& t_;
		
	public:
		typedef T type;
		
		reference_wrapper(T& t)
			: t_(t)
		{
		}
		
		operator T&()
		{
			return get();
		}
		
		T& get()
		{
			return t_;
		}
	};
	
	
	//=====================================================================
	// the two little functions that make for easy typing! usually, using
	// these two should be all that's required to make things work.
	//=====================================================================
	template <typename T>
	inline reference_wrapper<T> ref(T& t)
	{
		return reference_wrapper<T>(t);
	}
	
	template <typename T>
	inline reference_wrapper<T const> cref(const T& t)
	{
		return reference_wrapper<T const>(t);
	}
	
	
	//=====================================================================
	// you can make your algorithms aware of atma::ref, by using these
	// little metafunctions.
	//=====================================================================
	template <typename T>
	struct is_reference_wrapper
	{
		static const bool value = false;
	};
	
	template <typename T>
	struct is_reference_wrapper< reference_wrapper<T> >
	{
		static const bool value = true;
	};
	
	
	//=====================================================================
	// this is a very useful little metafunction. if you've got a type that
	// might be a reference, and you want to use its methods/operators, etc,
	// then use:
	//   typename unwrapped_ref<T>::type& temp_ref = instance_of_variable;
	// This will be a regular reference for normal types, but for
	// reference_wrapper, will allow us to have a reference to the base type.
	//=====================================================================
	template <typename T>
	struct unwrapped_ref
	{
		typedef T type;
	};
	
	template <typename T>
	struct unwrapped_ref< reference_wrapper<T> >
	{
		typedef T type;
	};
	
//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================