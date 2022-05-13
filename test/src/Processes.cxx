
#include <TRestDetectorSignalToRawSignalProcess.h>
#include <gtest/gtest.h>

using namespace std;

TEST(TRestDetectorSignalToRawSignalProcess, Default) {
    TRestDetectorSignalToRawSignalProcess process;

    EXPECT_TRUE(process.GetSampling() == 1.0);
    EXPECT_TRUE(process.GetNPoints() == 512);
    EXPECT_TRUE(process.GetTriggerMode() == "firstDeposit");
    EXPECT_TRUE(process.GetTriggerDelay() == 100);
    EXPECT_TRUE(process.GetGain() == 100.0);
    EXPECT_TRUE(process.GetIntegralThreshold() == 1229.0);

    process.PrintMetadata();
}
