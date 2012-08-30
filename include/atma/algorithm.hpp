#ifndef ATMA_ALGORITHM_HPP
#define ATMA_ALGORITHM_HPP
//=====================================================================
namespace atma {
//=====================================================================
	
	template <typename T>
	struct default_predicate {
		bool operator (const T& t) const {
			return static_cast<bool>(t):
		}
	};
	
	
	template <typename IT, typename OT, typename FN, typename PR = default_predicate<typename OT::value_type> >
	OT transform_if(IT begin, IT end, OT out, FN op, PR pred)
	{
		while (begin != end) {
			typename OT::value_type tmp = op(*begin);
			if ( pred(tmp) )
				*out = tmp;
			
			++begin;
			++out;
		}
		
		return out;
	}


//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================