//=====================================================================
//
//  CHANGE 'EMISSION' TO PRESENTER
//
//=====================================================================
#ifndef ATMA_UNITTEST_EMISSION_HPP
#define ATMA_UNITTEST_EMISSION_HPP
//=====================================================================
#include <string>
//=====================================================================
#include <atma/unittest/unittest.hpp>
//=====================================================================
namespace atma {
namespace unittest {
//=====================================================================
	
	//=====================================================================
	// this is the buffer-type we write everything to before streaming out
	//=====================================================================
	namespace detail
	{
		typedef std::list<std::string> buffer_t;
	}
	
	
	//=====================================================================
	// flat emission: just the checks line-per-line
	//=====================================================================
	namespace detail
	{
		namespace flat
		{
			namespace aux
			{
				bool order(const check_t& lhs, const check_t& rhs)
				{
					return lhs.file < rhs.file
						|| (lhs.file == rhs.file && lhs.line < rhs.line);
				}
			}
			
			inline void emit(buffer_t& buffer, checks_t::iterator begin, checks_t::iterator end)
			{
				
				std::stringstream stream;
				std::sort(begin, end, aux::order);
				
				for (checks_t::const_iterator i = begin; i != end; ++i)
				{
					if (!i->passed)
						stream
						<< i->file << "(" << i->line << "): "
						<< i->expression << "\n";
					buffer.push_back(stream.str());
					
					stream.str("");
				}
				
				
			}
		}
	}
	
	
	namespace detail
	{
		typedef void (*emitter_type)(buffer_t&, checks_t::iterator, checks_t::iterator);
		/*emitter_type& emitter()
		{
			static emitter_type emitter_ = flat::emitter;
			return emitter_;
		}*/
	}
	
	
	inline detail::emitter_type flat_emitter()
	{
		return detail::flat::emit;
	}
	
	
	//=====================================================================
	// entry point for emitting all results to somewhere
	//=====================================================================
	/*
	namespace detail
	{
		void emit_results(std::ostream& output, suite_map_t::iterator begin, suite_map_t::iterator end)
		{
			buffer_t buffer;
			(*emitter())(buffer, begin, end);
			
			for (buffer_t::const_iterator i = buffer.begin(); i != buffer.end(); ++i)
			{
				output << *i;
			}
		}
	}
	*/
	
//=====================================================================
} // namespace unittest
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

