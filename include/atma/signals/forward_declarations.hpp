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
#ifndef ATMA_SIGNALS_FORWARD_DECLARATIONS_HPP
#define ATMA_SIGNALS_FORWARD_DECLARATIONS_HPP
//=====================================================================
#include <boost/type_traits.hpp>
#include <boost/mpl/if.hpp>
//=====================================================================
#include "atma/atma__core.h"
//=====================================================================
ATMA_BEGIN
//=====================================================================

	//=====================================================================
	// an exception to use when someone signals with no slots bound
	//=====================================================================
	struct empty_signal_exception
		: std::exception
	{
		empty_signal_exception()
		 : std::exception("Attempt to signal with no bound slots")
		{
		}
	};

	//=====================================================================
	//
	//  last_value_combiner
	//  ---------------------
	//    A combiner who solely provides the last value outputted.
	//
	//=====================================================================
	template <typename RT>
	struct last_value_combiner;
	
		
	
	
	//=====================================================================
	// signal_base
	//=====================================================================
	namespace detail
	{
		template 
			<
			   typename FT,
			   typename CMB,
			   size_t Args = boost::function_traits<FT>::arity //detail::signal_arg_count<FT>::value
			>
		class signal_base;
	}
	
	
	//=====================================================================
	// signal
	//=====================================================================
	template <
			   typename FT,
			   typename CMB = last_value_combiner<boost::function_traits<FT>::result_type>
			 >
	class signal;
	
	
	//=====================================================================
	// slot_base
	//=====================================================================
	namespace detail
	{
		template <
				   typename FT, 
				   int Args = boost::function_traits<FT>::arity
				 > 
		class slot_base;
		
		template <typename FT, size_t A> struct abstract_slot_delegate;
		template <typename FT, typename T, size_t A> struct slot_delegate;
	}
	
	
	//=====================================================================
	// slot
	//=====================================================================
	template <typename FT>
	class slot;
	
	
	//=====================================================================
	//
	//  abstract_signal_slot_pair
	//
	//    and
	//
	//  signal_slot_pair
	//  ----------------------------
	//    abstract_signal_slot_pair provides a typeless interface to the
	//    typed signal_slot_pair. This lets connections (which are typeless)
	//    perform actions on it without needing to know the types of the
	//    slots and signals.
	//
	//=====================================================================
	namespace detail
	{
		struct abstract_signal_slot_pair;
		
		template < typename FT, typename CMB >
		struct signal_slot_pair;
	}
	
	
	//=====================================================================
	//
	//  connection_base
	//  -----------------
	//    A connection has a many-to-one relationship with a
	//    signal_slot_pair. This allows us to disconnect from one of our
	//    connections and have the signal_slot_pair detach all the other
	//    connections.
	//
	//=====================================================================
	namespace detail
	{
		class connection_base;
	}
	
	class connection;
	
	
	

//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif
//=====================================================================


