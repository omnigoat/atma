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
#ifndef ATMA_SLOT_HEADER_HPP
#define ATMA_SLOT_HEADER_HPP
//=====================================================================
#include "forward_declarations.hpp" // for slot_base
//=====================================================================
ATMA_BEGIN
//=====================================================================
	
	template <typename FT>
	class slot
	 : public detail::slot_base<FT>
	{
	public:
		// copy constructor
		slot(const slot& other);
		// constructor
		template <typename T> slot(T function);
		// equality operator
		bool operator == (const slot<FT>& rhs);
	};
	
	
//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif
//=====================================================================

