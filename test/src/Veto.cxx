
#include <TRestDetectorSignalEvent.h>
#include <TRestDetectorSignalToRawSignalProcess.h>
#include <TRestGeant4Event.h>
#include <TRestGeant4Metadata.h>
#include <TRestGeant4ToDetectorSignalVetoProcess.h>
#include <TRestRawSignalShapingProcess.h>
#include <TRestRun.h>
#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace std;

const auto filesPath = fs::path(__FILE__).parent_path().parent_path() / "files";
const auto vetoAnalysisRml = filesPath / "VetoAnalysis.rml";
const auto vetoAnalysisRestG4Run = filesPath / "CosmicMuonsSmall.root";

TEST(TRestGeant4ToDetectorSignalVetoProcess, TestFiles) {
    cout << "Test files path: " << filesPath << endl;

    // Check dir exists and is a directory
    EXPECT_TRUE(fs::is_directory(filesPath));
    // Check it's not empty
    EXPECT_TRUE(!fs::is_empty(filesPath));

    // All used files in this tests
    EXPECT_TRUE(fs::exists(vetoAnalysisRml));
    EXPECT_TRUE(fs::exists(vetoAnalysisRestG4Run));
}

TEST(TRestGeant4ToDetectorSignalVetoProcess, Default) {
    TRestGeant4ToDetectorSignalVetoProcess process;

    process.PrintMetadata();

    EXPECT_TRUE(process.GetVetoVolumesExpression().IsNull());
    EXPECT_TRUE(process.GetVetoDetectorExpression().IsNull());
    EXPECT_TRUE(process.GetVetoDetectorOffsetSize() == 0);
    EXPECT_TRUE(process.GetVetoLightAttenuation() == 0);
    EXPECT_TRUE(process.GetVetoQuenchingFactor() == 1.0);
}

TEST(TRestGeant4ToDetectorSignalVetoProcess, FromRml) {
    TRestGeant4ToDetectorSignalVetoProcess process(vetoAnalysisRml.c_str());

    process.PrintMetadata();

    EXPECT_TRUE(process.GetVetoVolumesExpression() == "^scintillatorVolume");
    EXPECT_TRUE(process.GetVetoDetectorExpression() == "^scintillatorLightGuideVolume");
    EXPECT_TRUE(process.GetVetoDetectorOffsetSize() == 0);
    EXPECT_TRUE(process.GetVetoLightAttenuation() == 0);
    EXPECT_TRUE(process.GetVetoQuenchingFactor() == 0);
}

TEST(TRestGeant4ToDetectorSignalVetoProcess, Simulation) {
    TRestGeant4ToDetectorSignalVetoProcess process(vetoAnalysisRml.c_str());

    TRestRun run(vetoAnalysisRestG4Run.c_str());
    run.GetInputFile()->ls();

    const auto metadata =
        dynamic_cast<const TRestGeant4Metadata*>(run.GetMetadataClass("TRestGeant4Metadata"));
    EXPECT_TRUE(metadata != nullptr);

    process.SetGeant4Metadata(metadata);

    process.InitProcess();
    process.PrintMetadata();

    EXPECT_TRUE(run.GetEntries() > 0);

    TRestGeant4Event* event = new TRestGeant4Event();
    run.SetInputEvent(event);
    run.GetEntry(0);

    EXPECT_TRUE(event != nullptr);
    // event->PrintEvent(1, 1);

    const auto outputEvent = (TRestDetectorSignalEvent*)process.ProcessEvent(event);
    EXPECT_TRUE(outputEvent != nullptr);
    // outputEvent->PrintEvent();

    // To Raw
    TRestDetectorSignalToRawSignalProcess processToRaw(vetoAnalysisRml.c_str());
    processToRaw.InitProcess();
    processToRaw.PrintMetadata();

    const auto outputEventRaw = (TRestRawSignalEvent*)processToRaw.ProcessEvent(outputEvent);
    EXPECT_TRUE(outputEventRaw != nullptr);
    // outputEventRaw->PrintEvent();

    // Shaping
    TRestRawSignalShapingProcess processShaping(vetoAnalysisRml.c_str());
    processShaping.InitProcess();
    processShaping.PrintMetadata();

    const auto outputEventShaping = (TRestRawSignalEvent*)processShaping.ProcessEvent(outputEventRaw);
    EXPECT_TRUE(outputEventShaping != nullptr);
    outputEventShaping->PrintEvent();
}
