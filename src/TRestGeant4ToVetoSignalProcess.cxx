//
// Created by lobis on 10/19/2021.
//

/* Example usage

<?xml version="1.0" encoding="UTF-8" standalone="no" ?>

<TRestManager>

    <TRestRun name="" title="" verboseLevel="info">
    <parameter name="outputFileName" value="test[fRunNumber]_[fRunTag].root"/>
    </TRestRun>

    <TRestProcessRunner name="TemplateEventProcess" verboseLevel="info">
    <parameter name="eventsToProcess" value="0"/>

    <parameter name="inputAnalysisStorage" value="off"/>
    <parameter name="inputEventStorage" value="on"/>
    <parameter name="outputEventStorage" value="on"/>


    <addProcess type="TRestGeant4ToVetoSignalProcess" name="test" value="ON">
    <parameter name="test" value="11"/>
    </addProcess>

    </TRestProcessRunner>

    <addTask type="processEvents" value="ON"/>

</TRestManager>

*/

#include "TRestGeant4ToVetoSignalProcess.h"

#include <TRestDetectorSignalEvent.h>
#include <TRestGeant4Event.h>
#include <TRestGeant4Metadata.h>

#include "spdlog/spdlog.h"

TRestGeant4ToVetoSignalProcess::TRestGeant4ToVetoSignalProcess()
    : fRestGeant4Event(nullptr), fRestDetectorSignalEvent(new TRestDetectorSignalEvent) {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);
}

TRestGeant4ToVetoSignalProcess::TRestGeant4ToVetoSignalProcess(char* rmlFile)
    : TRestGeant4ToVetoSignalProcess() {
    //
}

TRestGeant4ToVetoSignalProcess::~TRestGeant4ToVetoSignalProcess() = default;

void TRestGeant4ToVetoSignalProcess::Initialize() {}

void TRestGeant4ToVetoSignalProcess::InitFromConfigFile() {
    auto test = GetDblParameterWithUnits("test", (double)0);
    metadata << "TRestGeant4ToVetoSignalProcess::InitFromConfigFile - test parameter: " << test << endl;
}

void TRestGeant4ToVetoSignalProcess::InitProcess() {
    //
    spdlog::info("TRestGeant4ToVetoSignalProcess::InitProcess");

    fRestGeant4Metadata = GetMetadata<TRestGeant4Metadata>();
}

void TRestGeant4ToVetoSignalProcess::BeginOfEventProcess(TRestEvent* eventInput) {
    fRestGeant4Event = (TRestGeant4Event*)eventInput;

    fRestDetectorSignalEvent->SetRunOrigin(fRestGeant4Event->GetRunOrigin());
    fRestDetectorSignalEvent->SetSubRunOrigin(fRestGeant4Event->GetSubRunOrigin());
    fRestDetectorSignalEvent->SetID(fRestGeant4Event->GetID());
    fRestDetectorSignalEvent->SetSubID(fRestGeant4Event->GetSubID());
    fRestDetectorSignalEvent->SetSubEventTag(fRestGeant4Event->GetSubEventTag());
    fRestDetectorSignalEvent->SetTimeStamp(fRestGeant4Event->GetTimeStamp());
    fRestDetectorSignalEvent->SetState(fRestGeant4Event->isOk());
}

void TRestGeant4ToVetoSignalProcess::EndOfEventProcess(TRestEvent*) {
    //
}

void TRestGeant4ToVetoSignalProcess::EndProcess() {
    spdlog::info("TRestGeant4ToVetoSignalProcess::EndProcess");
}

TRestEvent* TRestGeant4ToVetoSignalProcess::ProcessEvent(TRestEvent* eventInput) {
    for (int i = 0; i < fRestGeant4Event->GetNumberOfHits(); i++) {
        fRestDetectorSignalEvent->AddChargeToSignal(0, 0, 0);
    }

    fRestDetectorSignalEvent->SortSignals();

    return (TRestEvent*)fRestDetectorSignalEvent;
}

void TRestGeant4ToVetoSignalProcess::PrintMetadata() const {}
