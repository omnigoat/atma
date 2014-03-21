//=====================================================================
//
//
//
//=====================================================================
#include <list>
#include <string>
#include <map>
#include <iomanip>
#include <iostream>
//=====================================================================
#include <atma/unittest/unittest.hpp>
#include <atma/unittest/ennumerate.hpp>
#include <atma/unittest/compile.hpp>
#include <atma/unittest/emission.hpp>
//=====================================================================
namespace atma {
namespace unittest {
//=====================================================================

	class run_type
	{
		friend struct all_tests;
		friend struct suite_named;
		
		// no value = all suites
		std::string suite_name_;
		
		run_type(const std::string& SN)
			: suite_name_(SN)
		{
		}
	public:
		const std::string& suite_name() const { return suite_name_; }
	};
	
	struct all_tests
		: run_type
	{
		all_tests()
			: run_type( std::string() )
		{
		}
	};
	
	/*
		// not until it's requested or needed
	struct suite_named
		: run_type
	{
		suite_named(const std::string& suite_name)
			: run_type(suite_name)
		{
		}
	};
	*/
	

	inline results_t run(const run_type& run_type = all_tests(), const detail::emitter_type& emit_type = flat_emitter(), std::ostream& output = std::cerr)
	{
#ifdef TIME_ALL_THE_THINGS
		atma::time_t start = atma::time();
#endif

		//detail::output_stream_ptr() = &output;
		detail::checks_t checks;
		detail::buffer_t buffer;
		results_t results;
		
		// all suites
		if (run_type.suite_name() == std::string())
		{
			detail::compile(detail::tests().begin(), detail::tests().end(), checks);
			results = detail::ennumerate(checks.begin(), checks.end());
			(*emit_type)(buffer, checks.begin(), checks.end());
		}
		
#ifdef TIME_ALL_THE_THINGS
		atma::time_t end = atma::time();
#endif

		output << "\n-------------------------------------------------" << std::endl;
		if (results.checks_failed > 0)
			output
				<< "atma::unittest [FAIL]: " 
				<< results.tests_failed << "/" << results.tests_run << " tests failed" << std::endl;
		else
#ifdef TIME_ALL_THE_THINGS
			output 
				<< "Atma.UnitTest: " << results.tests_run << " tests passed in "
				<< std::setprecision(2) << std::fixed << atma::convert_time_to_seconds(end - start) << "s"
				<< std::endl;
#else
			output
				<< "atma::unittest [PASS]: " << results.tests_run << " tests passed" << std::endl;
#endif
		
		output << "-------------------------------------------------" << std::endl;
		for (detail::buffer_t::const_iterator i = buffer.begin(); i != buffer.end(); ++i)
		{
			output << "  " << *i;
		}
		output << std::endl;

		if (results.tests_failed > 30)
			output << "  (" << results.tests_failed << " tests failed)\n" << std::endl;

		return results;
	}

//=====================================================================
} // namespace unittest
} // namespace atma
//=====================================================================

