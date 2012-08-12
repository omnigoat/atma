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
#ifndef ATMA_SIGNAL_BASE_HPP
#define ATMA_SIGNAL_BASE_HPP
//=====================================================================
#include <list>
#include <map>
#include <set>
//=====================================================================
#include <boost/preprocessor.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
//=====================================================================
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
//=====================================================================
#include <atma/atma__core.h>
//=====================================================================
#include <atma/signals/forward_declarations.hpp>
#include <atma/signals/signal_traits.hpp>
#include <atma/signals/slot_header.hpp>
#include <atma/signals/connection_header.hpp>
//=====================================================================
ATMA_BEGIN
//=====================================================================

	//=====================================================================
	// Maximum number of arguments is twelve
	//=====================================================================
		#ifndef ATMA_SIGNALS_MAX_ARGS
			#define ATMA_SIGNALS_MAX_ARGS 1
		#endif



		template <typename RT>
		struct last_value_combiner
		{
			template <typename InputIterator>
			RT operator ()(InputIterator begin, InputIterator end) const
			{
				return *--end;
			}
		};

	namespace detail
	{
		template <typename, typename, typename, size_t>
		struct signal_send_delegate;
	}

	//=====================================================================
	// We set up the limits, tell BOOST_PP which file to iterate (this one),
	// and then tell it to go! Go you mighty stallion!
	//=====================================================================
		#define BOOST_PP_ITERATION_LIMITS (0, ATMA_SIGNALS_MAX_ARGS)
		#define BOOST_PP_FILENAME_1 <atma/signals/signal_base.hpp>
		#include BOOST_PP_ITERATE()


//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif // ATMA_SIGNAL_HPP
//=====================================================================
#else // BOOST_PP_IS_ITERATING (we are currently in an iteration)
//=====================================================================
namespace detail {
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

		// getting parameters
		#define GET_PARAMETER(z, n, text) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) OUR_TYPE(n) OUR_ARG(n)
		#define GET_PARAMETER_LIST(n) BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(n), GET_PARAMETER, ~)

	
	
	
	//=====================================================================
	//
	//=====================================================================
	template <typename R, typename FT, typename CMB>
	struct signal_send_delegate<R, FT, CMB, N>
	{
	private:
		typedef typename signal_traits<FT, CMB>::sspair_container_type slot_container_type;
		typedef typename signal_traits<FT, CMB>::sspair_type sspair_type;
	
	public:
		static R send(typename slot_container_type::iterator begin, 
			typename slot_container_type::iterator end BOOST_PP_COMMA_IF(N) GET_PARAMETER_LIST(N) )
		{
			// a vector of the results from each slot
			std::vector<R> results;
			// calling each slot, and pushing back the result
			for (slot_container_type::iterator i = begin; i != end; ++i)
			{
				if ( !(*i)->is_blocked() )
				{
					// downcast to typed signal_slot_pair
					boost::shared_ptr<sspair_type> ssp = boost::dynamic_pointer_cast<sspair_type>(*i);
					// pass signal, storing result
					results.push_back( (ssp->m_slot)(GET_ARG_LIST(N)) );
				}
			}
			// return the value of the combiner
			return CMB()(results.begin(), results.end());
		}
	};
	
	template <typename FT, typename CMB>
	struct signal_send_delegate<void, FT, CMB, N>
	{
	private:
		typedef typename signal_traits<FT, CMB>::sspair_container_type slot_container_type;
		typedef typename signal_traits<FT, CMB>::sspair_type sspair_type;
	
	public:
		static void send(typename slot_container_type::iterator begin, 
			typename slot_container_type::iterator end BOOST_PP_COMMA_IF(N) GET_PARAMETER_LIST(N))
		{
			// here, we don't worry with return results, because they're void
			for (slot_container_type::iterator i = begin; i != end; ++i)
			{
				if ( !(*i)->is_blocked() )
				{
					// downcast to typed signal_slot_pair
					boost::shared_ptr<sspair_type> ssp = boost::dynamic_pointer_cast<sspair_type>(*i);
					// pass signal
					(*ssp)( GET_ARG_LIST(N) );
				}
			}
		}
	};
	
	//=====================================================================
	//
	//  Specialisation of signal_base
	//  -------------------------------
	//    Because we can have a different number of parameters, we need a
	//    specialisation for each. Rather than do it by hand, we'll use
	//    BOOST_PP to do so.
	//
	//=====================================================================
	template <typename FT, typename CMB>
	class signal_base<FT, CMB, N>
	{
		friend struct detail::signal_slot_pair<FT, CMB>;
	protected:
		// internal constants for us
		enum { GroupAfter = -1  };
		enum { GroupBefore = -2 };
	
	//---------------------------------------------
	protected: // private typedefs
	//---------------------------------------------
		typedef signal_traits<FT, CMB> traits;
		
		typedef typename traits::function_type function_type;
		typedef typename traits::return_type return_type;
		
		typedef typename traits::slot_container_type slot_container_type;
		typedef typename traits::sspair_type sspair_type;
		typedef typename traits::sspair_container_type sspair_container_type;
	
	//---------------------------------------------
	public: // public typedefs
	//---------------------------------------------
		typedef typename traits::signal_type signal_type;
		typedef typename traits::slot_type slot_type;
		
	
	//---------------------------------------------	
	protected: // private data
	//---------------------------------------------
		// all our slots
		//sspair_container_type m_sspairs;
		// temporary slots we should keep
		//slot_container_type m_slots;
		sspair_container_type sspairs_;
		// highest group yet
		size_t highest_group_;
		
	//---------------------------------------------
	public: // public functions
	//---------------------------------------------
		//=====================================================================
		// Constructor
		//=====================================================================
		signal_base()
		 : highest_group_()
		{
		}

		//=====================================================================
		// Send a signal to our slots
		//=====================================================================
		RETURN_TYPE send(GET_PARAMETER_LIST(N))
		{
			if (sspairs_.empty()) throw empty_signal_exception();
			
			return detail::signal_send_delegate<return_type, FT, CMB, N>::send
				(
					sspairs_.begin(),
					sspairs_.end()
					BOOST_PP_COMMA_IF(N) GET_ARG_LIST(N)
				);
		}

		RETURN_TYPE operator()( GET_PARAMETER_LIST(N) )
		{
			return send( GET_ARG_LIST(N) );
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
} // namespace detail
//=====================================================================
#endif // BOOST_PP_IS_ITERATING
//=====================================================================

