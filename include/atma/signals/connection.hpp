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
#ifndef ATMA_SIGNALS_CONNECTION_HPP
#define ATMA_SIGNALS_CONNECTION_HPP
//=====================================================================
#include "connection_header.hpp"
#include "signal.hpp"
#include "slot.hpp"
//=====================================================================
ATMA_BEGIN
//=====================================================================

template <typename FT, typename CMB>
void detail::signal_slot_pair<FT, CMB>::disconnect()
{
	// each connection is disengaged
	for (typename connection_container_type::iterator i = m_connections.begin(); i != m_connections.end(); ++i)
	{
		// don't disengage ourselves just yet
		(*i)->m_shared_data.reset();
	}
	
	// find our sspair in the signal, and remove it
	for (typename signal_traits<FT, CMB>::sspair_container_type::iterator i = m_signal->sspairs_.begin();
		i != m_signal->sspairs_.end(); ++i)
	{
		if (i->get() == this)
		{
			m_signal->sspairs_.erase(i);
			break;
		}
	}
}

connection::connection()
 : m_shared_data()
{
}

//=====================================================================
// Constructor - creates a shared-data instantiation, which we then
// add this connection to.
//=====================================================================
//template <typename FT, typename CMB>
connection::connection( boost::shared_ptr<detail::abstract_signal_slot_pair>& ssp)
 : m_shared_data(ssp)
{
	m_shared_data->attach_connection(this);
}

//=====================================================================
// Destructor - remove this connection reference from the signal-slot
//=====================================================================
connection::~connection()
{
	if (m_shared_data) m_shared_data->detach_connection(this);
}



//=====================================================================
// Simple mutators
//=====================================================================
// blocking / unblocking
inline void connection::set_blocked(bool blocked) { if (is_connected()) m_shared_data->set_block(blocked); }
// disconnecting (no reconnecting through connections)
inline void connection::disconnect() { if (is_connected()) m_shared_data->disconnect(); }

//=====================================================================
// Simple accessors
//=====================================================================
inline bool connection::is_blocked() { return m_shared_data->is_blocked(); }
inline bool connection::is_connected() { return bool(m_shared_data); }

//=====================================================================
// Assignment operator
//=====================================================================
connection& connection::operator = (const connection& c)
{
	// self-check
	if (this == &c) return *this;
	// detach our previous connection
	if (m_shared_data) m_shared_data->detach_connection(this);
	// transfer
	m_shared_data = c.m_shared_data;
	// attach to our new signal
	if (m_shared_data) m_shared_data->attach_connection(this);
	// and return us
	return *this;
}


//=====================================================================
// Copy constructor
//=====================================================================
connection::connection(const connection& c)
{
	// lock mutex
	//boost::mutex::scoped_lock sl(m_mutex);
	// change shared data
	m_shared_data = c.m_shared_data;
	// attach us to the signal-slot pair
	if (m_shared_data) m_shared_data->attach_connection(this);
}





//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif
//=====================================================================
