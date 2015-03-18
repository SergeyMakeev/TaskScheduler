#include "Tests.h"
#include "../../TestFramework/UnitTest++/UnitTest++.h"
#include "../Scheduler.h"
#include "../Platform.h"

SUITE(CleanupTests)
{
	static const int OLD_VALUE = 1;
	static const int VALUE = 13;

TEST(AtomicSimpleTest)
{
	MT::AtomicInt test;
	test.Set(OLD_VALUE);
	CHECK(test.Get() == OLD_VALUE);

	int prevValue = test.Set(VALUE);
	CHECK(test.Get() == VALUE);
	CHECK(prevValue == OLD_VALUE);

	int nowValue = test.Inc();
	CHECK(nowValue == (VALUE+1));

	nowValue = test.Dec();
	CHECK(nowValue == VALUE);

	nowValue = test.Add(VALUE);
	CHECK(nowValue == (VALUE+VALUE));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
