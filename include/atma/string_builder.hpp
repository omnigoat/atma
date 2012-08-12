#ifndef ATMA_STRING_BUILDER_HPP
#define ATMA_STRING_BUILDER_HPP
//=====================================================================
#include <sstream>
#include <ostream>
//=====================================================================
#include <boost/lexical_cast.hpp>
//=====================================================================
namespace atma {
//=====================================================================

	struct string_builder
	{
		string_builder() {}

		template <typename T>
		string_builder(T input) {
			SS << input;
		}

		template <typename T>
		string_builder& operator () (T rhs) {
			SS << rhs;
			return *this;
		}
		
		template <typename T>
		string_builder& operator << (T rhs) {
			SS << rhs;
			return *this;
		}
		
		// was meant to allow us to use std::endl with our operator, but not yet, gotta figure out why...
		template <typename T>
		string_builder& operator ()(std::basic_ostream< T, std::char_traits<T> >&(*O)(std::basic_ostream< T, std::char_traits<T> >&))
		{
			SS << O;
		}
		
		operator std::string() {
			return SS.str();
		}

		friend std::ostream& operator << (std::ostream& lhs, const string_builder& rhs);

	private:
		std::stringstream SS;
	};

	inline std::ostream& operator << (std::ostream& lhs, const string_builder& rhs)
	{
		lhs << rhs.SS.str();
		return lhs;
	}
	
//=====================================================================
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================