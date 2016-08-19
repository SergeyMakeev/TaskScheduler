#ifndef UNITTEST_EXECUTE_TEST_H
#define UNITTEST_EXECUTE_TEST_H

#include <time.h>
#include "Config.h"
#include "ExceptionMacros.h"
#include "TestDetails.h"
#include "TestResults.h"
#include "MemoryOutStream.h"
#include "AssertException.h"
#include "CurrentTest.h"
#include "TimeHelpers.h"

#ifdef UNITTEST_NO_EXCEPTIONS
	#include "ReportAssertImpl.h"
#endif

#ifdef UNITTEST_POSIX
	#include "Posix/SignalTranslator.h"
#endif

namespace UnitTest {

template< typename T >
void ExecuteTest(T& testObject, TestDetails const& details, bool isMockTest)
{
	if (isMockTest == false)
		CurrentTest::Details() = &details;

	testObject.RunImpl();
}

}

#endif
