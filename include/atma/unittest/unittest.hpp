//=====================================================================
//
//
//
//=====================================================================
#ifndef ATMA_UNITTEST_UNITTEST_HPP
#define ATMA_UNITTEST_UNITTEST_HPP
//=====================================================================
#include <string>
#include <vector>
#include <sstream>
//=====================================================================
namespace atma {
namespace unittest {
//=====================================================================
	
	//=====================================================================
	// default suite name
	//=====================================================================
	const std::string stored_suite_name("global_scope");
	
	//=====================================================================
	// all tests are derived from this, so we can call run on all heterogeneously
	//=====================================================================
	namespace detail
	{
		struct check_t
		{
			std::string suite_name;
			std::string test_name;
			std::string file;
			std::string expression;
			
			int line;
			bool passed;

			check_t(const std::string& suite_name, const std::string& test_name, const std::string& file, 
				int line, const std::string& expression, bool passed)
				: suite_name(suite_name), test_name(test_name), file(file), 
					line(line), expression(expression), passed(passed)
			{
			}
		};
		
		typedef std::vector<check_t> checks_t;
	}
	
	
	//=====================================================================
	// because of the architecture of this system, we need a test_t to
	// inherit from for each ATMA_TEST
	//=====================================================================
	namespace detail
	{
		struct test_t
		{
			typedef std::vector<check_t> check_container_t;
			std::string test_name;
			checks_t* checks;
			
			test_t(const std::string& test_name)
				: test_name(test_name)
			{
			}

			void record_check(const check_t& c) {
				checks->push_back(c);
			}
			
			template <typename A, typename B>
			void record_check_equal(const std::string& suite_name, const std::string& test_name, const char* file, int line, const A& a, const B& b)
			{
				std::stringstream ss;
				bool passed = a == b;
				if (!passed) {
					ss << "Expected (" << a << "), but it was (" << b << ")" << std::endl;
				}
				checks->push_back(check_t(suite_name, test_name, file, line, ss.str(), passed));
			}
			
			void run(checks_t& checks) {
				this->checks = &checks;
				run_impl();
			}
			
			virtual void run_impl() = 0;
		};
	}
	
	//=====================================================================
	// we need a unified list of all checks. here it is!
	//=====================================================================
	namespace detail
	{
		typedef std::vector<test_t*> tests_t;

		inline tests_t& tests() {
			static tests_t list_;
			return list_;
		}
	}
	

	
	
	//=====================================================================
	// this adds a test to our list automagically, so we don't require
	// manual test registration
	//=====================================================================
	namespace detail
	{
		struct test_adder
		{
			template <typename T> test_adder(T* input)
			{
				tests().push_back(static_cast<test_t*>(input));
			}
		};
	}
	
	
	//=====================================================================
	// defines a suite of tests
	//=====================================================================
	#define ATMA_SUITE(suite_name)	\
		namespace atma_unittest_suite_##suite_name \
		{ \
			namespace atma { \
				namespace unittest { \
					const std::string stored_suite_name( #suite_name ); \
				} \
			} \
		} \
		 \
		namespace atma_unittest_suite_##suite_name
	
	
	//=====================================================================
	// defines a test (careful of test_name and testname)
	//=====================================================================
	#define ATMA_TEST(test_name) \
		struct test_##test_name \
			: ::atma::unittest::detail::test_t \
		{ \
			test_##test_name(const std::string& testname) \
				: ::atma::unittest::detail::test_t(testname) \
			{ \
			} \
			 \
			void run_impl(); \
		} instance_##test_name(#test_name); \
		 \
		::atma::unittest::detail::test_adder test_adder_##test_name(&instance_##test_name); \
		 \
		void test_##test_name::run_impl()
	
	#define ATMA_TEST_FIXTURE(fixture_name, test_name) \
		struct fixture_name##test_name##helper : ::atma::unittest::detail::test_t, fixture_name \
		{ \
			fixture_name##test_name##helper(const std::string& testname) \
				: ::atma::unittest::detail::test_t(testname) \
			{ \
			} \
			 \
		 	void run_impl(); \
		 } instance_##fixture_name##test_name(#test_name); \
		  \
		 ::atma::unittest::detail::test_adder test_adder_##fixture_name##test_name(&instance_##fixture_name##test_name); \
		  \
		 void fixture_name##test_name##helper::run_impl()
	
	//=====================================================================
	// defines a test that uses a fixture
	//=====================================================================
	//#define ATMA_TEXT_WITH_FIXTURE(test_name)
	#define ATMA_CHECK(expression) \
		record_check( ::atma::unittest::detail::check_t(atma::unittest::stored_suite_name, test_name, __FILE__, __LINE__, #expression, (expression)) )
		
	#define ATMA_CHECK_EQUAL(a, b) \
		record_check_equal( atma::unittest::stored_suite_name, test_name, __FILE__, __LINE__, (a), (b) );
	
	//=====================================================================
	// replicating 'ATMA_SUITE' to 'SUITE' (and others) unless asked not to
	//=====================================================================
	#if !defined(ATMA_UNITTEST_SUPPRESS_NAMESPACE_PROMOTION)
	#	if defined(SUITE)
	#		error 'SUITE' is already defined. atma couldn't bring itself to redefine it.
	#	elif defined(TEST)
	#		error 'TEST' is already defined. atma couldn't bring itself to redefine it.
	#	elif defined(CHECK)
	#		error 'CHECK' is already defined. atma couldn't bring itself to redefine it.
	#	elif defined(TEST_FIXTURE)
	#		error 'TEST_FIXTURE' is already defined. atma couldn't bring itself to redefine it.
	#	elif defined(CHECK_EQUAL)
	#		error 'CHECK_EQUAL' is already_defined. atma couldn't bring itself to redefint it.
	#	endif
	#
	#	define SUITE(a) ATMA_SUITE(a)
	#	define TEST(a) ATMA_TEST(a)
	#	define CHECK(a) ATMA_CHECK(a)
	#	define TEST_FIXTURE(fixture, test_name) ATMA_TEST_FIXTURE(fixture, test_name)
	#	define CHECK_EQUAL(a, b) ATMA_CHECK_EQUAL(a, b)
	#endif
	
	
//=====================================================================
} // namespace unittest
} // namespace atma
//=====================================================================
#endif // inclusion guard
//=====================================================================