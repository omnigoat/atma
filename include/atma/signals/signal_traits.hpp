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
#ifndef ATMA_SIGNAL_TRAITS_CONNECTION_HPP
#define ATMA_SIGNAL_TRAITS_CONNECTION_HPP
//=====================================================================
#include <list>
#include <set>
//=====================================================================
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
//=====================================================================
#include "forward_declarations.hpp"
//=====================================================================
ATMA_BEGIN
//=====================================================================

//=====================================================================
namespace detail {
//=====================================================================

	//=====================================================================
	//
	//  abstract_signal_slot_pair
	//  ----------------------------
	//    An abstraction (to hide types) for the relation between one
	//    signal and one slot. It can have multiple connections attached
	//    to it.
	//
	//=====================================================================
	struct abstract_signal_slot_pair
	{
		// are we blocked?
		virtual bool is_blocked() = 0;
		// block connection
		virtual void set_block(bool) = 0;
		// return the group the slot is in
		virtual size_t group() = 0;
		// disconnect connection
		virtual void disconnect() = 0;
		// attach a connection to us
		virtual void attach_connection(connection*) = 0;
		// detach a connectino from us
		virtual bool detach_connection(connection*) = 0;
	};

	typedef boost::shared_ptr<abstract_signal_slot_pair> abstract_sspair_ptr;

	struct sspair_pred
	{
		bool operator ()(const abstract_sspair_ptr& lhs, const abstract_sspair_ptr& rhs) const
		{
			return lhs->group() < rhs->group();
		}
	};
	
//=====================================================================
} // namespace detail
//=====================================================================

	template <typename FT, typename CMB>
	class signal_traits
	{
	public:
		// typedefs for signals
		typedef signal<FT, CMB> signal_type;
		
		// typedefs for slots
		typedef slot<FT> slot_type;
		typedef std::list<slot_type> slot_container_type;
		
		// typedefs for parameter types
		typedef typename boost::function_traits<FT>::result_type return_type;
		typedef FT function_type;
		typedef CMB combiner_type;
		
		// signal-slot pair types
		typedef std::multiset< boost::shared_ptr<detail::abstract_signal_slot_pair>, detail::sspair_pred > sspair_container_type;
		typedef detail::signal_slot_pair<FT, CMB> sspair_type;
		typedef detail::abstract_signal_slot_pair sspair_ptr_type;
		
	};

	
	
//=====================================================================
namespace detail {
//=====================================================================


	

	//=====================================================================
	//
	//  signal_slot_pair
	//  ------------------
	//    The typed implementation of abstract_signal_slot_pair. We need
	//    this to cover up the underlying types that were used (because
	//    the connection class isn't templated).
	//
	//=====================================================================
	template <typename FT, typename CMB>
	struct signal_slot_pair
	 : public abstract_signal_slot_pair, public boost::enable_shared_from_this<signal_slot_pair<FT, CMB> >
	{
	public:
		typedef typename signal_traits<FT, CMB>::signal_type signal_type;
		typedef typename signal_traits<FT, CMB>::slot_type slot_type;

		typedef std::list<connection*> connection_container_type;

	public:
		// all our connections
		connection_container_type m_connections;
		// our signal
		signal_type* m_signal;
		// our slot
		slot_type m_slot;
		// group the slot is in
		size_t m_group;
		// if we're blocked
		bool m_blocked;

	public:
		//=====================================================================
		// Constructor
		//=====================================================================
		signal_slot_pair(signal_type* signal, const slot_type& slot, size_t group)
		: m_signal(signal), m_slot(slot), m_group(group), m_blocked(false)
		{
		}

		//=====================================================================
		// Destructor
		//=====================================================================
		~signal_slot_pair()
		{
		}

		//=====================================================================
		// Attaches a connection to us, and holds it for later
		//=====================================================================
		void attach_connection(connection* c)
		{
			m_connections.push_back(c);
		}

		//=====================================================================
		// Detaches a connection from us, and removes it from our list
		//=====================================================================
		bool detach_connection(connection* c)
		{
			// create a reference for easier typing in the following line
			connection_container_type& cs = m_connections;
			// five "cs", instead of five "m_connection"s
			return cs.erase(std::remove(cs.begin(), cs.end(), c), cs.end()) != cs.end();
		}

		//=====================================================================
		// Block the signal-slot communication
		//=====================================================================
		inline void set_block(bool value)
		{
			m_blocked = value;
		}

		//=====================================================================
		// Returns if we're blocked or not
		//=====================================================================
		inline bool is_blocked()
		{
			return m_blocked;
		}

		//=====================================================================
		// What group is this slot (WRT the signal) in?
		//=====================================================================
		size_t group()
		{
			return m_group;
		}

		//=====================================================================
		// Disconnect the signal-slot pair
		//=====================================================================
		void disconnect();

		//=====================================================================
		// even needed (?)
		//=====================================================================
		template <typename FT2, typename CMB2>
		friend bool operator == (const signal_slot_pair<FT2, CMB2>& lhs, const slot<FT2>& rhs);

		template <typename FT2, typename CMB2>
		friend bool operator == (const slot<FT2>& lhs, const signal_slot_pair<FT2, CMB2>& rhs);
	};


	template <typename FT, typename CMB>
	bool operator == (const signal_slot_pair<FT, CMB>& lhs, const slot<FT>& rhs)
	{
		return lhs.m_slot == &rhs;
	}

	template <typename FT, typename CMB>
	bool operator == (const slot<FT>& lhs, const signal_slot_pair<FT, CMB>& rhs)
	{
		return rhs == lhs;
	}

//=====================================================================
} // namespace detail
//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif
//=====================================================================
