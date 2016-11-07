
// we're using Catch!
//
//  http://github.com/philsquared/Catch
//

// we support all of the C++11 features except noexcept
#define CATCH_CONFIG_CPP11_NULLPTR
#define CATCH_CONFIG_CPP11_NO_NOEXCEPT
#define CATCH_CONFIG_CPP11_NO_GENERATED_METHODS
#define CATCH_CONFIG_CPP11_IS_ENUM
#define CATCH_CONFIG_CPP11_TUPLE
#define CATCH_CONFIG_VARIADIC_MACROS

#include "../../vendor/catch/catch.hpp"


// used for canary_t
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/array/elem.hpp>
#include <map>

#if defined(CATCH_CONFIG_MAIN)

namespace atma { namespace unit_test {

	using namespace Catch;

	template <bool SectionTrace>
	struct atma_reporter_t : Catch::StreamingReporterBase
	{
		atma_reporter_t(ReporterConfig const& config)
			: StreamingReporterBase(config)
		{}

		virtual ~atma_reporter_t() {}

		static std::string getDescription()
		{
			return "totes best styled";
		}

		void testRunStarting(TestRunInfo const& testRunInfo) override
		{
			stream
				<< '\n'
				<< "========================================================\n"
				<< "Running tests for " << testRunInfo.name.substr(0, testRunInfo.name.find_first_of('.')) << "...\n"
				<< "========================================================\n"
				<< std::flush;
		}

		ReporterPreferences getPreferences() const override
		{
			ReporterPreferences prefs;
			prefs.shouldRedirectStdOut = false;
			return prefs;
		}

		void noMatchingTestCases(std::string const& spec) override
		{
			stream << "No test cases matched '" << spec << "'" << std::endl;
		}

		void assertionStarting(AssertionInfo const&) override
		{
		}

		void printHeaderString(std::string const& _string, std::size_t indent = 0) {
			std::size_t i = _string.find(": ");
			if (i != std::string::npos)
				i += 2;
			else
				i = 0;
			stream << Text(_string, TextAttributes()
				.setIndent(indent + i)
				.setInitialIndent(indent)) << "\n";
		}

		bool assertionEnded(AssertionStats const& stats) override
		{
			AssertionResult const& result = stats.assertionResult;

			bool printInfoMessages = true;

			if (!m_config->includeSuccessfulResults() && result.isOk())
			{
				if (result.getResultType() != ResultWas::Warning)
					return false;
				printInfoMessages = false;
			}

			if (SectionTrace && !sections_printed_)
			{
				auto it = m_sectionStack.begin() + 1;
				auto itEnd = m_sectionStack.end();
				for (; it != itEnd; ++it)
					printHeaderString(it->name, 2);
				sections_printed_ = true;
			}

			assertion_printer_t printer(stream, stats, printInfoMessages);
			printer.print();

			stream << std::endl;
			return true;
		}

		void testRunEnded(TestRunStats const& stats) override
		{
			printTotals(stats.totals);
			stream << "\n" << std::endl;
			StreamingReporterBase::testRunEnded(stats);
			sections_printed_ = false;
		}

	private:
		struct assertion_printer_t
		{
			assertion_printer_t(std::ostream& stream, AssertionStats const& stats, bool messages)
				: stream(stream)
				, stats(stats)
				, result(stats.assertionResult)
				, messages(stats.infoMessages)
				, itMessage(stats.infoMessages.begin())
				, printInfoMessages(messages)
			{}

			void operator = (assertion_printer_t const&) = delete;

			void print()
			{
				printSourceInfo();

				itMessage = messages.begin();

				switch (result.getResultType())
				{
					case ResultWas::Ok:
						printResultType(Colour::ResultSuccess, passedString());
						printOriginalExpression();
						printReconstructedExpression();
						if (!result.hasExpression())
							printRemainingMessages(Colour::None);
						else
							printRemainingMessages();
						break;

					case ResultWas::ExpressionFailed:
						if (result.isOk())
							printResultType(Colour::ResultSuccess, failedString() + std::string(" - but was ok"));
						else
							printResultType(Colour::Error, failedString());
						printOriginalExpression();
						printReconstructedExpression();
						printRemainingMessages();
						break;

					case ResultWas::ThrewException:
						printResultType(Colour::Error, failedString());
						printIssue("unexpected exception with message:");
						printMessage();
						printExpressionWas();
						printRemainingMessages();
						break;

					case ResultWas::FatalErrorCondition:
						printResultType(Colour::Error, failedString());
						printIssue("fatal error condition with message:");
						printMessage();
						printExpressionWas();
						printRemainingMessages();
						break;

					case ResultWas::DidntThrowException:
						printResultType(Colour::Error, failedString());
						printIssue("expected exception, got none");
						printExpressionWas();
						printRemainingMessages();
						break;

					case ResultWas::Info:
						printResultType(Colour::None, "info");
						printMessage();
						printRemainingMessages();
						break;

					case ResultWas::Warning:
						printResultType(Colour::None, "warning");
						printMessage();
						printRemainingMessages();
						break;

					case ResultWas::ExplicitFailure:
						printResultType(Colour::Error, failedString());
						printIssue("explicitly");
						printRemainingMessages(Colour::None);
						break;
						// These cases are here to prevent compiler warnings

					case ResultWas::Unknown:
					case ResultWas::FailureBit:
					case ResultWas::Exception:
						printResultType(Colour::Error, "** internal error **");
						break;
				}
			}

		private:
			// Colour::LightGrey

			static Colour::Code dimColour() { return Colour::FileName; }

			static const char* failedString() { return "failed"; }
			static const char* passedString() { return "passed"; }

			void printSourceInfo() const
			{
				Colour colourGuard(Colour::FileName);
				if (SectionTrace)
					stream << '\t';
				stream << '\t' << result.getSourceInfo() << ":";
			}

			void printResultType(Colour::Code colour, std::string passOrFail) const
			{
				if (!passOrFail.empty())
				{
					{
						Colour colourGuard(colour);
						stream << " " << passOrFail;
					}
					stream << ":";
				}
			}

			void printIssue(std::string issue) const
			{
				stream << " " << issue;
			}

			void printExpressionWas()
			{
				if (result.hasExpression())
				{
					stream << ";";
					{
						Colour colour(dimColour());
						stream << " expression was:";
					}
					printOriginalExpression();
				}
			}

			void printOriginalExpression() const
			{
				if (result.hasExpression()) {
					stream << " " << result.getExpression();
				}
			}

			void printReconstructedExpression() const
			{
				if (result.hasExpandedExpression())
				{
					{
						Colour colour(dimColour());
						stream << "  (expanded: ";
					}
					stream << result.getExpandedExpression();
					{
						Colour colour(dimColour());
						stream << ")";
					}
				}
			}

			void printMessage()
			{
				if (itMessage != messages.end()) {
					stream << " '" << itMessage->message << "'";
					++itMessage;
				}
			}

			void printRemainingMessages(Colour::Code colour = dimColour())
			{
				if (itMessage == messages.end())
					return;

				// using messages.end() directly yields compilation error:
				std::vector<MessageInfo>::const_iterator itEnd = messages.end();
				const std::size_t N = static_cast<std::size_t>(std::distance(itMessage, itEnd));

				{
					Colour colourGuard(colour);
					stream << " with " << pluralise(N, "message") << ":";
				}

				for (; itMessage != itEnd;) {
					// If this assertion is a warning ignore any INFO messages
					if (printInfoMessages || itMessage->type != ResultWas::Info) {
						stream << " '" << itMessage->message << "'";
						if (++itMessage != itEnd) {
							Colour colourGuard(dimColour());
							stream << " and";
						}
					}
				}
			}

		private:
			std::ostream& stream;
			AssertionStats const& stats;
			AssertionResult const& result;
			std::vector<MessageInfo> messages;
			std::vector<MessageInfo>::const_iterator itMessage;
			bool printInfoMessages;
		};

		// Colour, message variants:
		// - white: No tests ran.
		// -   red: Failed [both/all] N test cases, failed [both/all] M assertions.
		// - white: Passed [both/all] N test cases (no assertions).
		// -   red: Failed N tests cases, failed M assertions.
		// - green: Passed [both/all] N tests cases with M assertions.
		void printTotals(const Totals& totals) const
		{
			if (totals.testCases.total() == 0)
			{
				stream << "\tNo tests ran.";
			}
			else if (totals.testCases.failed == totals.testCases.total())
			{
				Colour colour(Colour::ResultError);
				stream
					<< "\n\tFailed " << pluralise(totals.testCases.failed, "test case")
					<< ", failed " << pluralise(totals.assertions.failed, "assertion") << ".";
			}
			else if (totals.assertions.total() == 0)
			{
				stream
					<< "\tPassed " << pluralise(totals.testCases.total(), "test case")
					<< " (no assertions).";
			}
			else if (totals.assertions.failed)
			{
				Colour colour(Colour::ResultError);
				stream
					<< "\n\tFailed " << pluralise(totals.testCases.failed, "test case")
					<< ", failed " << pluralise(totals.assertions.failed, "assertion") << ".";
			}
			else
			{
				Colour colour(Colour::ResultSuccess);
				stream
					<< "\tPassed " << pluralise(totals.testCases.passed, "test case")
					<< " with "  << pluralise(totals.assertions.passed, "assertion") << ".";
			}
		}

	private:
		bool sections_printed_ = false;
	};

	using atma_reporter_concise_t = atma_reporter_t<false>;
	using atma_reporter_extended_t = atma_reporter_t<true>;

	REGISTER_REPORTER("atma", atma_reporter_concise_t);
	REGISTER_REPORTER("atma_ex", atma_reporter_extended_t);

}}

#endif




namespace atma { namespace unit_test {

#ifndef CANARY_STDOUT
#	define CANARY_STDOUT 0
#endif

	enum class canary_oper_t : int
	{
		unknown = -1,
		default_constructor,
		direct_constructor,
		copy_constructor,
		move_constructor,
		destructor,
	};

	struct canary_t
	{
		struct event_t
		{
			event_t(canary_oper_t oper)
				: id(-1), oper(oper), payload(-1)
			{}

			event_t(int id, canary_oper_t oper)
				: id(id), oper(oper), payload(-1)
			{}

			event_t(int id, canary_oper_t oper, int payload)
				: id(id), oper(oper), payload(payload)
			{}

			int id;
			canary_oper_t oper;
			int payload;
		};

		struct scope_switcher_t
		{
			explicit scope_switcher_t(std::string const& name)
			{
				canary_t::switch_scope(name);
			}

			~scope_switcher_t()
			{
				//canary_t::switch_scope_nil();
			}

			operator bool() const { return true; }
		};

		using event_log_t = std::vector<event_t>;
		using event_log_map_t = std::map<std::string, std::pair<int, event_log_t>>;


		canary_t()
			: scope(current_scope())
			, id(generate_id())
			, payload()
		{
#if CANARY_STDOUT
			std::cout << "[" << scope->first << ':' << id << "] canary_t::default-constructor(" << payload << ')' << std::endl;
#endif
			scope->second.second.emplace_back(id, canary_oper_t::default_constructor, payload);
		}

		canary_t(int payload)
			: scope(current_scope())
			, id(generate_id())
			, payload(payload)
		{
#if CANARY_STDOUT
			std::cout << "[" << scope->first << ':' << id << "] canary_t::direct-constructor(" << payload << ')' << std::endl;
#endif
			scope->second.second.emplace_back(id, canary_oper_t::direct_constructor, payload);
		}

		canary_t(canary_t const& rhs)
			: scope(current_scope())
			, id(generate_id())
			, payload(rhs.payload)
		{
#if CANARY_STDOUT
			std::cout << "[" << scope->first << ':' << id << "] canary_t::copy-constructor(" << payload << ')' << std::endl;
#endif
			scope->second.second.emplace_back(id, canary_oper_t::copy_constructor, payload);
		}

		canary_t(canary_t&& rhs)
			: scope(current_scope())
			, id(generate_id())
			, payload(rhs.payload)
		{
			rhs.payload = 0;
#if CANARY_STDOUT
			std::cout << "[" << scope->first << ':' << id << "] canary_t::move-constructor(" << payload << ')' << std::endl;
#endif
			scope->second.second.emplace_back(id, canary_oper_t::move_constructor, payload);
		}

		~canary_t()
		{
#if CANARY_STDOUT
			std::cout << "[" << scope->first << ':' << id << "] canary_t::destructor(" << payload << ')' << std::endl;
#endif
			scope->second.second.emplace_back(id, canary_oper_t::destructor, payload);
		}



		static auto event_log_matches(std::string const& name, std::vector<event_t> r) -> bool
		{
			auto const& event_log = event_log_handle(name)->second.second;

			if (r.size() != event_log.size())
				return false;

			auto e = event_log.begin();
			for (auto i = r.begin(); i != r.end(); ++i, ++e)
			{
				if (e->id != -1 && i->id != -1 && e->id != i->id)
					return false;

				if (e->oper != canary_oper_t::unknown && i->oper != canary_oper_t::unknown && e->oper != i->oper)
					return false;

				if (e->payload != -1 && i->payload != -1 && e->payload != i->payload)
					return false;
			}

			return true;
		}

		static auto event_log_matches(std::vector<event_t> r) -> bool
		{
			auto const& event_log = current_scope()->second.second; // event_log_handle(name)->second.second;

			if (r.size() != event_log.size())
				return false;

			auto e = event_log.begin();
			for (auto i = r.begin(); i != r.end(); ++i, ++e)
			{
				if (e->id != -1 && i->id != -1 && e->id != i->id)
					return false;

				if (e->oper != canary_oper_t::unknown && i->oper != canary_oper_t::unknown && e->oper != i->oper)
					return false;

				if (e->payload != -1 && i->payload != -1 && e->payload != i->payload)
					return false;
			}

			return true;
		}

		int payload;

	private:
		event_log_map_t::iterator scope;
		int id;

	private:
		static auto event_log_map() -> event_log_map_t&
		{
			thread_local static event_log_map_t logs;
			return logs;
		}

		static auto event_log_handle(std::string const& name) -> event_log_map_t::iterator
		{
			auto& map = event_log_map();

			auto it = map.find(name);
			if (it == map.end())
				it = map.insert({name, std::make_pair(0, event_log_t{})}).first;

			return it;
		}

		static event_log_map_t::iterator& current_scope()
		{
			static event_log_map_t::iterator _;
			return _;
		}

		static auto switch_scope(std::string const& name) -> void
		{
			current_scope() = event_log_handle(name);
			current_scope()->second.second.clear();
			current_scope()->second.first = 0;
		}

		static auto switch_scope_nil() -> void
		{
			current_scope() = event_log_map().end();
		}

		static int generate_id()
		{
			return ++current_scope()->second.first;
		}
	};

	struct canary_event_checker_t
	{
		using events_t = std::vector<canary_t::event_t>;

		canary_event_checker_t()
		{}

		~canary_event_checker_t()
		{
			CHECK(canary_t::event_log_matches(events_));
		}

		operator bool() const { return true; }

		auto default_constructor(int id, int payload = -1) { events_.emplace_back(id, canary_oper_t::default_constructor, payload); }
		auto direct_constructor(int id, int payload = -1)  { events_.emplace_back(id, canary_oper_t::direct_constructor, payload); }
		auto copy_constructor(int id, int payload = -1)    { events_.emplace_back(id, canary_oper_t::copy_constructor, payload); }
		auto move_constructor(int id, int payload = -1)    { events_.emplace_back(id, canary_oper_t::move_constructor, payload); }
		auto destructor(int id, int payload = -1)          { events_.emplace_back(id, canary_oper_t::destructor, payload); }

	private:
		events_t events_;
	};

	inline auto operator == (canary_t const& lhs, int rhs) -> bool
	{
		return lhs.payload == rhs;
	}

	inline auto operator == (canary_t const& lhs, canary_t const& rhs) -> bool
	{
		return lhs.payload == rhs.payload;
	}

#define CHECK_CANARY_SCOPE(name, ...) \
	CHECK(::atma::unit_test::canary_t::event_log_matches(name, {__VA_ARGS__}))

#define CANARY_CC_ORDER(...) \
	BOOST_PP_TUPLE_ENUM((__VA_ARGS__))

#define CANARY_SWITCH_SCOPE(name) \
	if (auto S = ::atma::unit_test::canary_t::scope_switcher_t(name))

#define THEN_CANARY \
	THEN("canary event log matches") \
	if (auto& C = ::atma::unit_test::canary_event_checker_t{})

#define GIVEN_CANARY(name) \
	GIVEN(name) \
	CANARY_SWITCH_SCOPE(name)




#define CHECK_VECTOR_II_m(r, v, i, elem) \
	CHECK(BOOST_PP_ARRAY_ELEM(0, v)[i]BOOST_PP_ARRAY_ELEM(1, v) == elem);

#define CHECK_VECTOR_II(v, expr, seq) \
	BOOST_PP_SEQ_FOR_EACH_I(CHECK_VECTOR_II_m, (2, (v, expr)), seq)

#define CHECK_VECTOR_EX(v, expr, ...) \
	CHECK_VECTOR_II(v, expr, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define CHECK_VECTOR(v, ...) \
	CHECK_VECTOR_EX(v, , __VA_ARGS__)

#define CHECK_WHOLE_VECTOR(v, ...) \
	CHECK(v.size() == BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)); \
	CHECK_VECTOR_EX(v, , __VA_ARGS__)


}}




