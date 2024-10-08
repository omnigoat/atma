#define DOCTEST_CONFIG_IMPLEMENT
#include <atma/unit_test.hpp>

#include <atma/functor.hpp>
#include <atma/assert.hpp>
#include <array>
#include <memory>

int main(int argc, char** argv)
{
	return doctest::Context(argc, argv).run();
}

