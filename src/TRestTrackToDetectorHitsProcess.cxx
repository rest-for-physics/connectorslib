//////////////////////////////////////////////////////////////////////////
///
///             RESTSoft : Software for Rare Event Searches with TPCs
///
///             TRestTrackToDetectorHitsProcess.cxx
///
///             Apr 2017:   First concept (Javier Galan)
///
//////////////////////////////////////////////////////////////////////////

#include "TRestTrackToDetectorHitsProcess.h"

using namespace std;

ClassImp(TRestTrackToDetectorHitsProcess);

TRestTrackToDetectorHitsProcess::TRestTrackToDetectorHitsProcess() { Initialize(); }

TRestTrackToDetectorHitsProcess::TRestTrackToDetectorHitsProcess(const char* configFilename) {
    Initialize();

    if (LoadConfigFromFile(configFilename) == -1) LoadDefaultConfig();
}

TRestTrackToDetectorHitsProcess::~TRestTrackToDetectorHitsProcess() { delete fOutputHitsEvent; }

void TRestTrackToDetectorHitsProcess::LoadDefaultConfig() {
    SetName("trackToDetectorHitsProcess");
    SetTitle("Default config");

    fTrackLevel = 0;
}

void TRestTrackToDetectorHitsProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fInputTrackEvent = nullptr;
    fOutputHitsEvent = new TRestDetectorHitsEvent();
}

void TRestTrackToDetectorHitsProcess::LoadConfig(const string& configFilename, const string& name) {
    if (LoadConfigFromFile(configFilename, name) == -1) LoadDefaultConfig();
}

void TRestTrackToDetectorHitsProcess::InitProcess() {}

TRestEvent* TRestTrackToDetectorHitsProcess::ProcessEvent(TRestEvent* inputEvent) {
    fInputTrackEvent = (TRestTrackEvent*)inputEvent;

    if (this->GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug) fInputTrackEvent->PrintOnlyTracks();

    for (int n = 0; n < fInputTrackEvent->GetNumberOfTracks(); n++)
        if (fInputTrackEvent->GetLevel(n) == fTrackLevel) {
            TRestHits* hits = fInputTrackEvent->GetTrack(n)->GetHits();

            for (int h = 0; h < hits->GetNumberOfHits(); h++)
                fOutputHitsEvent->AddHit(hits->GetX(h), hits->GetY(h), hits->GetZ(h), hits->GetEnergy(h),
                                         hits->GetTime(h), hits->GetType(h));
        }

    return fOutputHitsEvent;
}

void TRestTrackToDetectorHitsProcess::EndProcess() {}

void TRestTrackToDetectorHitsProcess::InitFromConfigFile() {
    fTrackLevel = StringToInteger(GetParameter("trackLevel", "1"));
}
