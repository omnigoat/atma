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
#ifndef ATMA_SIGNAL_HEADER_HPP
#define ATMA_SIGNAL_HEADER_HPP
//=====================================================================
#include "forward_declarations.hpp"
#include "signal_traits.hpp"
//=====================================================================
ATMA_BEGIN
//=====================================================================

	template <typename FT, typename CMB>
	class signal
	 : public detail::signal_base<FT, CMB>
	{
	private:
		typedef signal_traits<FT, CMB> traits;
		
		typedef typename traits::function_type function_type;
		typedef typename traits::return_type return_type;
		
		typedef typename traits::slot_container_type slot_container_type;
		typedef typename traits::sspair_type sspair_type;
		typedef typename traits::sspair_container_type sspair_container_type;
	
	public:
		typedef typename traits::signal_type signal_type;
		typedef typename traits::slot_type slot_type;
	
	public:
		template <typename T>
		connection connect(const T& slot, int group = GroupAfter);
		void disconnect_all_slots();
		
	private:
		// internally connect a slot to us
		connection connect_(slot_type& slot, int group);
	};

//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif
//=====================================================================

