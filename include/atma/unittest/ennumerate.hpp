//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_UNITTEST_ENNUMERATE_HPP
#define ATMA_UNITTEST_ENNUMERATE_HPP
//=====================================================================
#include <algorithm>
//=====================================================================
#include <atma/unittest/unittest.hpp>
//=====================================================================
namespace atma {
namespace unittest {
//=====================================================================	

	struct results_t
	{
		int suites_run;
		int suites_failed;

		int tests_run;
		int tests_failed;

		int checks_performed;
		int checks_failed;


		results_t() : suites_run(), suites_failed(), tests_run(), tests_failed(), checks_performed(), checks_failed() {}
	};
	
	//=====================================================================
	// orders checks by suite_name->test_name->file-no. good for ennumerate
	//=====================================================================
	namespace detail
	{
		namespace aux
		{
			bool enum_order(const check_t& lhs, const check_t& rhs)
			{
				return lhs.suite_name < rhs.suite_name
					|| (lhs.suite_name == rhs.suite_name && lhs.test_name < rhs.test_name)
					|| (lhs.test_name == rhs.test_name && lhs.line < rhs.line);
			}
		}
	}
	
	
	//=====================================================================
	// ennumeration!
	//=====================================================================
	namespace detail
	{
		inline results_t ennumerate(checks_t::iterator begin, checks_t::iterator end)
		{
			results_t results;
			if (begin == end) return results;
			
			std::sort(begin, end, &aux::enum_order);
			
			
			std::string current_suite = begin->suite_name;
			std::string current_test = begin->test_name;
			size_t current_suite_failed_tests = 0;
			size_t current_test_failed_checks = 0;
			
			for (checks_t::iterator i = begin; i != end; ++i) {
				if (i->suite_name != current_suite) {
					++results.suites_run;
					if (current_suite_failed_tests > 0) {
						++results.suites_failed;
						current_suite_failed_tests = 0;
					}
					
					current_suite = i->suite_name;
				}
				
				if (i->test_name != current_test) {
					++results.tests_run;
					if (current_test_failed_checks > 0) {
						++current_suite_failed_tests;
						++results.tests_failed;
						current_test_failed_checks = 0;
					}
					current_test = i->test_name;
				}
				
				if (!i->passed) {
					++results.checks_failed;
					++current_test_failed_checks;
				}
				
				++results.checks_performed;
			}
			
			// we perform this since we won't ennumerate the last suite in the list otherwise
			++results.suites_run;
			if (current_suite_failed_tests > 0) {
				++results.suites_failed;
				current_suite_failed_tests = 0;
			}
			
			return results;
		}
	}
	
//=====================================================================
} // namespace unittest
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================

