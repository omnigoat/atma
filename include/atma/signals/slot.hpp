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
#ifndef ATMA_SLOT_HPP
#define ATMA_SLOT_HPP
//=====================================================================
#include "forward_declarations.hpp"
#include "slot_header.hpp"
#include "slot_base.hpp"
//=====================================================================
ATMA_BEGIN
//=====================================================================

	//=====================================================================
	//
	//  slot implementation
	//  ---------------------
	//    Man, just having this here is kind of pedantic.
	//
	//=====================================================================
	template <typename FT>
	slot<FT>::slot(const slot& other)
	 : slot_base<FT>(static_cast<const slot_base&>(other))
	{
	}
	
	template <typename FT>
	template <typename T>
	slot<FT>::slot(T t)
	 : slot_base<FT>(t)
	{
	}

	template <typename FT>
	bool slot<FT>::operator == (const slot<FT>& rhs)
	{
		return delegate_->equal(*rhs.delegate_);
	}


//=====================================================================
ATMA_CLOSE
//=====================================================================
#endif // ATMA_SLOT_HPP
//=====================================================================

