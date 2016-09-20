#include "TestReporterStdout.h"
#include <cstdio>

#include "TestDetails.h"

#ifdef _XBOX_ONE
#include <windows.h>
#include <assert.h>
#endif

// cstdio doesn't pull in namespace std on VC6, so we do it here.
#if defined(UNITTEST_WIN32) && (_MSC_VER == 1200)
	namespace std {}
#endif

namespace UnitTest {

void TestReporterStdout::ReportFailure(TestDetails const& details, char const* failure)
{
#ifdef _XBOX_ONE
	details;
	failure;
	OutputDebugStringA("Failed ");
	OutputDebugStringA(failure);
	OutputDebugStringA("\n");
	//assert(false);
#else

    using namespace std;
#if defined(__APPLE__) || defined(__GNUG__)
    char const* const errorFormat = "%s:%d:%d: error: Failure in %s: %s\n";
    fprintf(stderr, errorFormat, details.filename, details.lineNumber, 1, details.testName, failure);
#else
    char const* const errorFormat = "%s(%d): error: Failure in %s: %s\n";
    fprintf(stderr, errorFormat, details.filename, details.lineNumber, details.testName, failure);
#endif
#endif
}

void TestReporterStdout::ReportTestStart(TestDetails const& details/*test*/)
{
#ifdef _XBOX_ONE
	OutputDebugStringA("\n=> Test: ");
	OutputDebugStringA(details.testName);
	OutputDebugStringA("\n");
#else
	printf("\n=> Test: %s\n", details.testName);
#endif
}

void TestReporterStdout::ReportTestFinish(TestDetails const& /*test*/, float delta)
{
#ifdef _XBOX_ONE
	char msg[128];
	sprintf(msg, "=> Finished in %3.2f sec\n", delta);
	OutputDebugStringA(msg);
#else
	printf("=> Finished in %3.2f sec\n", delta);
#endif
}

void TestReporterStdout::ReportSummary(int const totalTestCount, int const failedTestCount,
                                       int const failureCount, float const secondsElapsed)
{
	using namespace std;

    if (failureCount > 0)
        printf("FAILURE: %d out of %d tests failed (%d failures).\n", failedTestCount, totalTestCount, failureCount);
    else
        printf("Success: %d tests passed.\n", totalTestCount);

#ifdef _XBOX_ONE
	char msg[128];
	sprintf(msg, "Test time: %.2f seconds.\n", secondsElapsed);
	OutputDebugStringA(msg);
#else
    printf("Test time: %.2f seconds.\n", secondsElapsed);
#endif
}

}
