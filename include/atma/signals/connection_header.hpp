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
#ifndef ATMA_SIGNALS_CONNECTION_HEADER_HPP
#define ATMA_SIGNALS_CONNECTION_HEADER_HPP
//=====================================================================
#include <vector>
//=====================================================================
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
//=====================================================================
#include <atma/atma__core.h>
#include "forward_declarations.hpp"
#include "signal_traits.hpp"
#include "signal_header.hpp"
#include "slot_header.hpp"
//=====================================================================
ATMA_BEGIN
//=====================================================================
	
	class connection
	{
		template <typename FT, typename CMB> friend class signal;
		template <typename FT, typename CMB> friend struct detail::signal_slot_pair;
		
	public:
		//=====================================================================
		// Constructor/Destructor/Assignment/Copy-Constructor
		//=====================================================================
		inline connection();
		virtual inline ~connection();
		
		inline connection& operator = (const connection& rhs);
		inline connection( const connection& );
		
		//=====================================================================
		// Simple mutators
		//=====================================================================
		// blocking / unblocking
		inline void set_blocked(bool blocked);
		// disconnecting (no reconnecting through connections)
		inline void disconnect();
		
		//=====================================================================
		// Simple accessors
		//=====================================================================
		inline bool is_blocked();
		inline bool is_connected();
			
	protected:
		//=====================================================================
		// Constructor - creates a shared-data instantiation, which we then
		// add this connection to.
		//=====================================================================
		inline connection(boost::shared_ptr<detail::abstract_signal_slot_pair>&);
		
	protected:
		// a connection
		boost::shared_ptr<detail::abstract_signal_slot_pair> m_shared_data;
	};

	

//=====================================================================

ATMA_CLOSE
//=====================================================================
#endif
//=====================================================================
