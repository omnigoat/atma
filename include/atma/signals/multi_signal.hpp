//=====================================================================
//
//






// this don't work, and probably shouldn't exist (ie, the functionality
// isn't really required). but it was hax when it did work, until I
// changed things for the better, and couldn't be bothered fixing it.
// I mean, it's quite complex.






//    The Atma.Signals Library
//    ----------------------------
//        This library was written as a replacement of Boost.Signal,
//    so we do not need to link to the static library. As it stands, it
//    is not thread-safe, so please don't come crying to us if you try
//    and use it as such. 
//
//====================================================================
#ifndef BOOST_PP_IS_ITERATING
//#####################################################################
//#
//#    Pre-Iterating Code
//#    ----------------------
//#
//#####################################################################
#ifndef ATMA_MULTI_SIGNAL_HPP
#define ATMA_MULTI_SIGNAL_HPP
//=====================================================================
#include <boost/type_traits.hpp>
#include <boost/mpl/comparison.hpp>
#include <boost/mpl/inherit.hpp>
//=====================================================================
#include <boost/preprocessor.hpp>
//=====================================================================
#include <atma/atma__core.h>
#include "signal.hpp"
//=====================================================================
ATMA_BEGIN 
namespace detail {
//=====================================================================

	// setup - set up the limits and whatnot of the Boost.PP
	#define ATMA_MULTI_SIGNAL_LIMIT 16
	#define BOOST_PP_ITERATION_LIMITS (0, ATMA_MULTI_SIGNAL_LIMIT )
	#define BOOST_PP_FILENAME_1 <atma/signals/multi_signal.hpp>
	
	// iterate - this sets of a stacked iteration of the same file. Cleverness!
	#include BOOST_PP_ITERATE()

	// post-iterating - after all the iterating, parse this code
	#define ATMA_MULTI_SIGNAL_END

//=====================================================================
} // namespace detail
ATMA_CLOSE
//=====================================================================
#endif // ATMA_MULTI_SIGNAL_HPP
//=====================================================================

//#####################################################################
//#
//#    Iterating Code
//#    ------------------
//#
//#####################################################################
#else // BOOST_PP_IS_ITERATING (we are currently in an iteration)
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

	// declaring types
	#define GET_TYPE(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) typename T##n
	#define GET_TYPE_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_TYPE, ~)
	
	// type names
	#define GET_TYPE_NAME(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) T##n
	#define GET_TYPE_NAME_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_TYPE_NAME, ~)
	
//=====================================================================
//
//  inheritor
//  -----------
//    Inherits N things
//
//=====================================================================
	#if (N > 0)
		template< GET_TYPE_LIST(N) >
	#endif
		BOOST_PP_CAT(struct inheritor, N)
	#if (N > 0)
			: GET_TYPE_NAME_LIST(N)
	#endif
		{
		};

//=====================================================================
//
//  recursive_tail_uninheritor
//  ----------------------------
//    Removes RM from the tail of a list of things to inherit, assuming
//    TN is the same type as RM. Keeps going until another type is
//    encountered. We don't have one for zero arguments, obviously
//
//=====================================================================
	#if (N > 0)
		template< typename RM , GET_TYPE_LIST(N) > 
		BOOST_PP_CAT(struct recursive_tail_uninheritor, N)
			: boost::mpl::if_c
				<
					boost::is_same< RM , BOOST_PP_CAT(T,N) >::value, 
				#if (N == 1)
					inheritor0,
				#else
					BOOST_PP_CAT(recursive_tail_uninheritor, BOOST_PP_DEC(N)) < RM, GET_TYPE_NAME_LIST(BOOST_PP_DEC(N)) > ,
				#endif
					BOOST_PP_CAT(inheritor,N) < GET_TYPE_NAME_LIST(N) >
				>::type
		{
		};
	#endif

//=====================================================================
//
//  Undefining The Macros
//  -----------------------
//    Clean up after ourself. Yes sir.
//
//=====================================================================
	#undef N
	#undef GET_TYPE
	#undef GET_TYPE_LIST
	#undef GET_TYPE_NAME
	#undef GET_TYPE_NAME_LIST

//=====================================================================
#endif // BOOST_PP_IS_ITERATING
//=====================================================================




//#####################################################################
//#
//#    Post-Iterating Code
//#    -----------------------
//#
//#####################################################################
#ifdef ATMA_MULTI_SIGNAL_END
//=====================================================================
ATMA_BEGIN
//=====================================================================
//
//  Macros
//  --------
//    We are going to define a few macros to make everything really
//    easy for us. These will be undefined at the end of the thing, so
//    they're not THAT evil.
//
//=====================================================================
	// template parameters
	#define GET_TYPE(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) typename BOOST_PP_CAT(T, n) BOOST_PP_IF(n, = void, BOOST_PP_EMPTY())
	#define GET_TYPE_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_TYPE, ~)

	// signals
	#define GET_SIGNAL(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) signal< BOOST_PP_CAT(T, n) >
	#define GET_SIGNAL_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_SIGNAL, ~)
	
	// recursive_tail_uninheritor
	#define GET_RTU() BOOST_PP_CAT(detail::recursive_tail_uninheritor, ATMA_MULTI_SIGNAL_LIMIT)
	
	
	
	//=====================================================================
	//
	//  multi_signal
	//  ----------
	//    This is the product of a lot of work.
	//
	//=====================================================================
	template < GET_TYPE_LIST( ATMA_MULTI_SIGNAL_LIMIT ) > 
	class multi_signal
	 : public GET_RTU() < signal<void>, GET_SIGNAL_LIST( ATMA_MULTI_SIGNAL_LIMIT ) >
	{
	public:
		template <typename EFT>
		connection connect(slot<EFT>& the_function)
		{
			return signal<EFT>::connect(the_function);
		}
	};

//=====================================================================
//
//  Undefining The Macros
//  -----------------------
//    Clean up after ourself. Yes sir.
//
//=====================================================================
	#undef N
	#undef GET_TYPE
	#undef GET_TYPE_LIST
	#undef GET_SIGNAL
	#undef GET_SIGNAL_LIST
	#undef GET_RTU
	#undef ATMA_MULTI_SIGNAL_LIMIT

//=====================================================================
ATMA_CLOSE
//=====================================================================
#undef ATMA_MULTI_SIGNAL_END // yes, we actually need this
#endif // ATMA_MULTI_SIGNAL_END
//=====================================================================

