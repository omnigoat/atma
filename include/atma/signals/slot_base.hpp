//=====================================================================
//
//
//    The Atma.Signals Library
//    ----------------------------
//        This library was written as a replacement of Boost.Signal,
//    so we do not need to link to the static library. As it stands, it
//    is not thread-safe, so please don't come crying to us if you try
//    and use it as such.
//
//=====================================================================
#ifndef BOOST_PP_IS_ITERATING
//=====================================================================
#ifndef ATMA_SLOT_BASE_HPP
#define ATMA_SLOT_BASE_HPP
//=====================================================================
#include <boost/type_traits.hpp>
#include <boost/preprocessor.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>
//=====================================================================
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
//=====================================================================
#include <atma/atma__core.h>
#include "forward_declarations.hpp"
//=====================================================================
ATMA_BEGIN
//=====================================================================

	//=====================================================================
	// Maximum number of arguments is twelve
	//=====================================================================
		#ifndef ATMA_SIGNALS_MAX_ARGS
			#define ATMA_SIGNALS_MAX_ARGS 1
		#endif


	//=====================================================================
	// We set up the limits, tell BOOST_PP which file to iterate (this one),
	// and then tell it to go! Go you mighty stallion!
	//=====================================================================
		#define BOOST_PP_ITERATION_LIMITS (0, ATMA_SIGNALS_MAX_ARGS)
		#define BOOST_PP_FILENAME_1 <atma/signals/slot_base.hpp>
		#include BOOST_PP_ITERATE()

//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif // ATMA_SLOT_BASE_HPP
//=====================================================================
#else // BOOST_PP_IS_ITERATING (we are currently in an iteration)
//=====================================================================

	//=====================================================================
	//
	//  Macros
	//  --------
	//    We are going to define a few macros to make everything really
	//    easy for us. These will be undefined at the end of the thing, so
	//    they're not THAT evil.
	//
	//=====================================================================
		// which iteration are we in?
		#define N BOOST_PP_ITERATION()

		// return types
		#define RETURN_TYPE typename boost::function_traits<FT>::result_type

		// getting types
		#define OUR_TYPE(n) typename boost::function_traits<FT>::arg##n##_type
		#define GET_TYPE(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) OUR_TYPE(n)
		#define GET_TYPE_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_TYPE, ~)

		// getting arguments
		#define OUR_ARG(n) a##n
		#define GET_ARG(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) OUR_ARG(n)
		#define GET_ARG_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_ARG, ~)
		#define ARG_LIST GET_ARG_LIST(N)
		
		// getting parameters
		#define GET_PARAMETER(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) OUR_TYPE(n) OUR_ARG(n)
		#define GET_PARAMETER_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_PARAMETER, ~)
		#define PARAMETER_LIST GET_PARAMETER_LIST(N)


	//=====================================================================
	//
	//  abstract_slot_delegate
	//  ------------------------
	//    Used for slot, ey?
	//
	//=====================================================================
	namespace detail
	{
		template <typename FT>
		struct abstract_slot_delegate<FT, N>
		{
			virtual RETURN_TYPE operator ()(PARAMETER_LIST) = 0;
			virtual bool equal(const abstract_slot_delegate<FT, N>& rhs) = 0;
		};
	}
	
	//=====================================================================
	//
	//  slot_delegate
	//  ------------------------
	//    Used for slot, ey?
	//
	//=====================================================================
	namespace detail
	{
		template <typename FT, typename T>
		struct slot_delegate<FT, T, N>
		 : detail::abstract_slot_delegate<FT, N>
		{
			T t;
			
			slot_delegate(T t)
			 : t(t)
			{
			}
			
			RETURN_TYPE operator()(PARAMETER_LIST)
			{
				return t();
			}
			
			bool equal(const abstract_slot_delegate<FT, N>& rhs)
			{
				return dynamic_cast<const slot_delegate<FT, T, N>*>(&rhs) != NULL;
			}
		};
	}
	
	//=====================================================================
	//
	//  Specialisation of slot
	//  ------------------------
	//    Because we can have a different number of parameters, we need a
	//    specialisation for each. Rather than do it by hand, we'll use
	//    BOOST_PP to do so.
	//
	//=====================================================================
	namespace detail
	{
		template <typename FT>
		class slot_base<FT, N>
		{
		protected:
			typedef detail::abstract_slot_delegate<FT, N> slot_delegate_type;
			typedef boost::shared_ptr<slot_delegate_type> delegate_ptr_type;
			delegate_ptr_type delegate_;
			
		public:
			//=====================================================================
			// constructor / destructor
			//=====================================================================
			template <typename T>
			slot_base(T function)
			{
				delegate_ = delegate_ptr_type(new detail::slot_delegate<FT, T, N>(function));
			}
			
			slot_base(const slot_base& other)
			{
				delegate_ = other.delegate_;
			}
			
			virtual ~slot_base()
			{
			}

			//=====================================================================
			// sending
			//=====================================================================
			RETURN_TYPE operator ()(PARAMETER_LIST)
			{
				return (*delegate_)(ARG_LIST);
			}
		};
	}
	
	//=====================================================================
	//
	//  Undefining The Macros
	//  -----------------------
	//    Clean up after ourself. Yes sir.
	//
	//=====================================================================
		#undef N
		#undef RETURN_TYPE
		#undef OUR_TYPE
		#undef GET_TYPE
		#undef GET_TYPE_LIST
		#undef OUR_ARG
		#undef GET_ARG
		#undef GET_ARG_LIST
		#undef GET_PARAMETER
		#undef GET_PARAMETER_LIST


//=====================================================================
#endif // BOOST_PP_IS_ITERATING
//=====================================================================

