
#include <string.h>
#include "gtest/gtest.h"
#include "osal/osal.h"
#include "utils/platform_log.h"

LOG_MODNAME("test_random.cpp");

#if defined(WIN32) || defined(__linux__)// || defined(__APPLE__) || defined(__APPLE__)
const int ITERATIONS = 10000;
#else
const int ITERATIONS = 200;
#endif

TEST( RANDOM, CompareRandomStrings ) {
    OSALInit();
    const int arrsz = 128;
    const int iterations = ITERATIONS;
    uint8_t random1[arrsz];
    uint8_t random2[arrsz];
    bool success = true;

    LOG_TRACE(("Test random starting...\r\n"));
    const uint32_t time1 = OSALGetMS();
    OSALRandom( random1, arrsz );

    for (int i = 0; (success) && (i < iterations); i++){
        OSALRandom( random2, arrsz );
        bool equals = (0 == memcmp( random1, random2, arrsz ) );
        if (equals) {
            success = false;
            LOG_TRACE(("random1 == random2 at iteration %d\r\n", i));
        }
        if (0 == (i & 0x1fff)){
            LOG_TRACE(("...iterations = %u of %u\r\n", i, iterations));
        }
        EXPECT_TRUE( success );
    }
    const uint32_t time2 = OSALGetMS();
    const uint32_t timepassed = time2 - time1;
    LOG_TRACE(("Test random took %ums for %d iterations. Success = %d\r\n", timepassed, iterations, success));

}

