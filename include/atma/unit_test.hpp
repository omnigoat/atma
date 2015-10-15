
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

#if defined(CATCH_CONFIG_MAIN)

namespace atma { namespace unit_test {

	using namespace Catch;

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
				<< "Running tests for " << testRunInfo.name << '\n'
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
	};


	REGISTER_REPORTER("atma", atma_reporter_t);

}}

#endif

