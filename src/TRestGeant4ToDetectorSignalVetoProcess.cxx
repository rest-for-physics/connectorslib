
#include "TRestGeant4ToDetectorSignalVetoProcess.h"

#include <fmt/color.h>
#include <fmt/core.h>

using namespace fmt;
using namespace std;

ClassImp(TRestGeant4ToDetectorSignalVetoProcess);

TRestGeant4ToDetectorSignalVetoProcess::TRestGeant4ToDetectorSignalVetoProcess() { Initialize(); }

TRestGeant4ToDetectorSignalVetoProcess::TRestGeant4ToDetectorSignalVetoProcess(const char* configFilename) {
    TRestGeant4ToDetectorSignalVetoProcess();
    if (LoadConfigFromFile(configFilename)) {
        LoadDefaultConfig();
    }
}

TRestGeant4ToDetectorSignalVetoProcess::~TRestGeant4ToDetectorSignalVetoProcess() { delete fOutputEvent; }

void TRestGeant4ToDetectorSignalVetoProcess::LoadDefaultConfig() { SetTitle("Default config"); }

void TRestGeant4ToDetectorSignalVetoProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fOutputEvent = new TRestDetectorSignalEvent();
}

void TRestGeant4ToDetectorSignalVetoProcess::LoadConfig(const string& configFilename, const string& name) {
    if (LoadConfigFromFile(configFilename, name)) LoadDefaultConfig();
}

void TRestGeant4ToDetectorSignalVetoProcess::InitProcess() {
    // CAREFUL THIS METHOD IS CALLED TWICE!
    fVetoVolumes.clear();
    fVetoDetectorVolumes.clear();
    fVetoDetectorBoundaryDirection.clear();
    fVetoDetectorBoundaryPosition.clear();

    if (fGeant4Metadata == nullptr) {
        // maybe it was manually initialized
        fGeant4Metadata = GetMetadata<TRestGeant4Metadata>();
    }
    if (fGeant4Metadata == nullptr) {
        cerr << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Geant4 metadata not found" << endl;
        exit(1);
    }

    const auto& geometryInfo = fGeant4Metadata->GetGeant4GeometryInfo();

    fVetoVolumes = geometryInfo.GetAllPhysicalVolumesMatchingExpression(fVetoVolumesExpression);
    if (fVetoVolumes.empty()) {
        const auto logicalVolumes =
            geometryInfo.GetAllLogicalVolumesMatchingExpression(fVetoVolumesExpression);
        for (const auto& logicalVolume : logicalVolumes) {
            for (const auto& physicalVolume : geometryInfo.GetAllPhysicalVolumesFromLogical(logicalVolume)) {
                fVetoVolumes.push_back(geometryInfo.GetAlternativeNameFromGeant4PhysicalName(physicalVolume));
            }
        }
    }
    if (fVetoVolumes.empty()) {
        cerr << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: No veto volumes found" << endl;
        exit(1);
    }

    // get detector volumes if requested
    if (!fVetoDetectorsExpression.IsNull()) {
        fVetoDetectorVolumes = geometryInfo.GetAllPhysicalVolumesMatchingExpression(fVetoDetectorsExpression);
        if (fVetoDetectorVolumes.empty()) {
            const auto logicalVolumes =
                geometryInfo.GetAllLogicalVolumesMatchingExpression(fVetoDetectorsExpression);
            for (const auto& logicalVolume : logicalVolumes) {
                for (const auto& physicalVolume :
                     geometryInfo.GetAllPhysicalVolumesFromLogical(logicalVolume)) {
                    fVetoDetectorVolumes.push_back(
                        geometryInfo.GetAlternativeNameFromGeant4PhysicalName(physicalVolume));
                }
            }
        }
        if (fVetoDetectorVolumes.empty()) {
            cerr << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: No detector volumes found" << endl;
            exit(1);
        }
        if (fVetoDetectorVolumes.size() != fVetoVolumes.size()) {
            cerr << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Number of detector volumes "
                 << "does not match number of veto volumes" << endl;
            exit(1);
        }
    }

    for (int i = 0; i < fVetoDetectorVolumes.size(); i++) {
        const auto& vetoName = fVetoVolumes[i];
        const auto& vetoPosition = geometryInfo.GetPosition(vetoName);

        const auto& vetoDetectorName = fVetoDetectorVolumes[i];
        const auto& vetoDetectorPosition = geometryInfo.GetPosition(vetoDetectorName);

        const auto distance = vetoDetectorPosition - vetoPosition;
        const auto direction = distance.Unit();

        fVetoDetectorBoundaryDirection[vetoName] = direction;
        fVetoDetectorBoundaryPosition[vetoName] = vetoDetectorPosition - direction * fVetoDetectorOffsetSize;
    }

    PrintMetadata();
}

TRestEvent* TRestGeant4ToDetectorSignalVetoProcess::ProcessEvent(TRestEvent* inputEvent) {
    fInputEvent = (TRestGeant4Event*)inputEvent;
    *fOutputEvent = TRestDetectorSignalEvent();

    fOutputEvent->SetID(fInputEvent->GetID());
    fOutputEvent->SetSubID(fInputEvent->GetSubID());
    fOutputEvent->SetTimeStamp(fInputEvent->GetTimeStamp());
    fOutputEvent->SetSubEventTag(fInputEvent->GetSubEventTag());

    map<TString, TRestDetectorSignal> fVetoSignalMap;
    int vetoID = 0;
    for (const auto& volume : fVetoVolumes) {
        fVetoSignalMap[volume].SetSignalID(vetoID++);
    }
    for (const auto& track : fInputEvent->GetTracks()) {
        const auto& hits = track.GetHits();
        for (int i = 0; i < hits.GetNumberOfHits(); i++) {
            const auto volume = fGeant4Metadata->GetGeant4GeometryInfo().GetVolumeFromID(hits.GetVolumeId(i));
            if (fVetoSignalMap.count(volume) <= 0) {
                continue;
            }
            const auto energy = hits.GetEnergy(i);
            if (energy <= 0) {
                // TODO: quenching
                continue;
            }
            auto& signal = fVetoSignalMap.at(volume);
            const auto time = hits.GetTime(i);
            signal.AddPoint(time, energy);
        }
    }
    for (auto& [id, signal] : fVetoSignalMap) {
        if (signal.GetNumberOfPoints() <= 0) {
            continue;
        }
        fOutputEvent->AddSignal(signal);
    }

    return fOutputEvent;
}

void TRestGeant4ToDetectorSignalVetoProcess::EndProcess() {}

void TRestGeant4ToDetectorSignalVetoProcess::InitFromConfigFile() {
    // word to identify active volume as veto (default = "veto" e.g. "vetoTop")
    fVetoVolumesExpression = GetParameter("vetoVolumesExpression", fVetoVolumesExpression);
    fVetoDetectorsExpression = GetParameter("vetoDetectorsExpression", fVetoDetectorsExpression);

    fVetoDetectorOffsetSize = GetDblParameterWithUnits("vetoDetectorOffset", fVetoDetectorOffsetSize);
    fVetoLightAttenuation = GetDblParameterWithUnits("vetoLightAttenuation", fVetoLightAttenuation);
    fVetoQuenchingFactor = GetDblParameterWithUnits("quenchingFactor", fVetoQuenchingFactor);
}

// TODO: Find how to place this so that we don't need to copy it in every source file
template <>
struct fmt::formatter<TVector3> : formatter<string> {
    auto format(TVector3 c, format_context& ctx) {
        string s = fmt::format("({:0.3f}, {:0.3f}, {:0.3f})", c.X(), c.Y(), c.Z());
        return formatter<string>::format(s, ctx);
    }
};

void TRestGeant4ToDetectorSignalVetoProcess::PrintMetadata() {
    BeginPrintProcess();

    cout << "Veto volume expression: " << fVetoVolumesExpression << endl;
    if (!fVetoDetectorsExpression.IsNull()) {
        cout << "Veto detector expression: " << fVetoDetectorsExpression << endl;
        cout << "Veto detector offset: " << fVetoDetectorOffsetSize << endl;
        cout << "Veto light attenuation: " << fVetoLightAttenuation << endl;
    } else {
        cout << "Veto detector expression: not set" << endl;
    }
    cout << "Veto quenching factor: " << fVetoQuenchingFactor << endl;

    RESTDebug << RESTendl;

    if (fVetoVolumes.empty()) {
        cout << "Process not initialized yet" << endl;
        return;
    }

    cout << "Number of veto volumes: " << fVetoVolumes.size() << endl;
    cout << "Number of veto detector volumes: " << fVetoDetectorVolumes.size() << endl;

    const auto& geometryInfo = fGeant4Metadata->GetGeant4GeometryInfo();
    for (int i = 0; i < fVetoVolumes.size(); i++) {
        const auto& vetoName = fVetoVolumes[i];
        const auto& vetoPosition = geometryInfo.GetPosition(vetoName);

        print(" - Veto volume: {} - name: '{}' - position: {} mm\n", i, vetoName, vetoPosition);

        if (fVetoDetectorVolumes.empty()) {
            continue;
        }

        const auto& vetoDetectorName = fVetoDetectorVolumes[i];
        const auto& vetoDetectorPosition = geometryInfo.GetPosition(vetoDetectorName);

        print("   Veto detector name: '{}' - position: {} mm\n", vetoDetectorName, vetoDetectorPosition);

        print("   Boundary position: {} mm - direction: {}\n", fVetoDetectorBoundaryPosition.at(vetoName),
              fVetoDetectorBoundaryDirection.at(vetoName));
    }
    EndPrintProcess();
}
