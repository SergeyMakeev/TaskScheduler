#include "Tests.h"
#include "../Test/UnitTest++/UnitTest++.h"


TEST(TestConcurrency)
{
	CHECK(true);
}


int Tests::RunAll()
{
	return UnitTest::RunAllTests();
}
