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
#ifndef ATMA_SIGNAL_HPP
#define ATMA_SIGNAL_HPP
//=====================================================================
#include "signal_header.hpp"
#include "signal_base.hpp"
#include "slot.hpp"
#include "connection.hpp"
//=====================================================================
ATMA_BEGIN
//=====================================================================

	//=====================================================================
	// public
	//=====================================================================
	template <typename FT, typename CMB>
	template <typename T>
	connection signal<FT, CMB>::connect(const T& s, int group = GroupAfter)
	{
		//m_slots.push_back(slot);
		return connect_(slot<FT>(s), group);
	}
	
	
	template <typename FT, typename CMB>
	void signal<FT, CMB>::disconnect_all_slots()
	{
		// the list is invalidated every time we call disconnect, so this way is best
		while (!sspairs_.empty())
			(*sspairs_.begin())->disconnect();
	}
	
	
	
	//=====================================================================
	// protected
	//=====================================================================
	template <typename FT, typename CMB>
	connection signal<FT, CMB>::connect_(slot_type& slot, int group)
	{
		sspair_container_type::iterator result;
		
		// place at beginning
		if (group == GroupBefore)
		{
			result = sspairs_.insert(boost::shared_ptr<detail::abstract_signal_slot_pair>(new sspair_type(this, slot, 0)));
		}
		// place at back
		else if (group == GroupAfter)
		{
			result = sspairs_.insert(boost::shared_ptr<detail::abstract_signal_slot_pair>(new sspair_type(this, slot, highest_group_)));
		}
		// insertion in right order
		else if (group >= 0)
		{
			result = sspairs_.insert(boost::shared_ptr<detail::abstract_signal_slot_pair>(new sspair_type(this, slot, group)));
		}
		// weird value (group < -2): bork
		else
		{
			throw std::exception("Y'all screwed up with slot-group");
		}
		
		return connection(*result);
	}


	
	
//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif // ATMA_SIGNAL_HPP
//=====================================================================


