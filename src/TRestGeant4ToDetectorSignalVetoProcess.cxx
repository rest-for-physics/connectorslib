
#include "TRestGeant4ToDetectorSignalVetoProcess.h"

using namespace std;

ClassImp(TRestGeant4ToDetectorSignalVetoProcess);

TRestGeant4ToDetectorSignalVetoProcess::TRestGeant4ToDetectorSignalVetoProcess() { Initialize(); }

TRestGeant4ToDetectorSignalVetoProcess::TRestGeant4ToDetectorSignalVetoProcess(const char* configFilename)
    : TRestGeant4ToDetectorSignalVetoProcess() {
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
    fVetoVolumesToSignalIdMap.clear();

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

    for (int i = 0; i < fVetoVolumes.size(); i++) {
        fVetoVolumesToSignalIdMap[fVetoVolumes[i]] = i;
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

    // Drift
    if (fDriftEnabled) {
        // Check if selected volumes are valid and replace them by the physical volume if user set the logical
        if (!geometryInfo.IsValidPhysicalVolume(fDriftVolume)) {
            if (geometryInfo.IsValidLogicalVolume(fDriftVolume)) {
                if (geometryInfo.GetAllPhysicalVolumesFromLogical(fDriftVolume).size() == 1) {
                    fDriftVolume = geometryInfo.GetAlternativeNameFromGeant4PhysicalName(
                        geometryInfo.GetAllPhysicalVolumesFromLogical(fDriftVolume)[0]);
                } else {
                    cout << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Logical volume "
                         << fDriftVolume
                         << " has more than one physical volume. Please explicitly select the physical volume"
                         << endl;
                    exit(1);
                }
            } else {
                cout << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Volume " << fDriftVolume
                     << " is not a valid physical or logical volume" << endl;
                exit(1);
            }
        }
        if (!geometryInfo.IsValidPhysicalVolume(fDriftReadoutVolume)) {
            if (geometryInfo.IsValidLogicalVolume(fDriftReadoutVolume)) {
                if (geometryInfo.GetAllPhysicalVolumesFromLogical(fDriftReadoutVolume).size() == 1) {
                    fDriftReadoutVolume = geometryInfo.GetAlternativeNameFromGeant4PhysicalName(
                        geometryInfo.GetAllPhysicalVolumesFromLogical(fDriftReadoutVolume)[0]);
                } else {
                    cout << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Logical volume "
                         << fDriftReadoutVolume
                         << " has more than one physical volume. Please explicitly select the physical volume"
                         << endl;
                    exit(1);
                }
            } else {
                cout << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Volume " << fDriftReadoutVolume
                     << " is not a valid physical or logical volume" << endl;
                exit(1);
            }
        }
        if (fDriftVelocity <= 0) {
            cout << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Drift velocity must be positive"
                 << endl;
            exit(1);
        }
        if (fDriftReadoutNormalDirection.Mag() == 0) {
            cout << "TRestGeant4ToDetectorSignalVetoProcess::InitProcess: Drift readout normal direction "
                    "cannot be zero"
                 << endl;
            exit(1);
        }
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

    // If drift is enabled compute the delay
    Double_t triggerTime = 0;
    if (fDriftEnabled) {
        const auto& geometryInfo = fGeant4Metadata->GetGeant4GeometryInfo();
        const auto& readoutVolumePositionWithOffset = geometryInfo.GetPosition(fDriftReadoutVolume) +
                                                      fDriftReadoutNormalDirection * fDriftReadoutOffset;
        for (const auto& track : fInputEvent->GetTracks()) {
            const auto& hits = track.GetHits();
            for (int i = 0; i < hits.GetNumberOfHits(); i++) {
                const auto volume =
                    fGeant4Metadata->GetGeant4GeometryInfo().GetVolumeFromID(hits.GetVolumeId(i));
                if (volume != fDriftVolume) {
                    continue;
                }
                auto energy = hits.GetEnergy(i);
                if (energy <= 0) {
                    continue;
                }
                const TVector3 position = hits.GetPosition(i);
                const double distance = fDriftReadoutNormalDirection * position -
                                        fDriftReadoutNormalDirection * readoutVolumePositionWithOffset;
                if (distance < 0) {
                    cout << "Distance to readout should never be negative" << endl;
                    exit(1);
                }
                const double hitTriggerTime = distance / fDriftVelocity + hits.GetTime(i);  // in us
                if (triggerTime == 0 || hitTriggerTime < triggerTime) {
                    triggerTime = hitTriggerTime;
                }
            }
        }
    }
    map<TString, TRestDetectorSignal> fVetoSignalMap;
    for (const auto& volume : fVetoVolumes) {
        fVetoSignalMap[volume].SetSignalID(fVetoVolumesToSignalIdMap.at(volume));
    }
    for (const auto& track : fInputEvent->GetTracks()) {
        const auto& hits = track.GetHits();
        for (int i = 0; i < hits.GetNumberOfHits(); i++) {
            const auto volume = fGeant4Metadata->GetGeant4GeometryInfo().GetVolumeFromID(hits.GetVolumeId(i));
            if (fVetoSignalMap.count(volume) <= 0) {
                continue;
            }
            auto energy = hits.GetEnergy(i);
            if (energy <= 0) {
                continue;
            }
            if (fVetoQuenchingFactor < 1) {
                const auto particle = track.GetParticleName();
                if (fParticlesNotQuenched.count(particle) == 0) {
                    energy *= fVetoQuenchingFactor;
                }
            }
            if (fVetoDetectorOffsetSize != 0 && fVetoLightAttenuation > 0) {
                const TVector3 position = hits.GetPosition(i);
                const double distance =
                    fVetoDetectorBoundaryPosition.at(volume) * fVetoDetectorBoundaryDirection.at(volume) -
                    position * fVetoDetectorBoundaryDirection.at(volume);
                assert(
                    distance >=
                    0.0);  // distance can never be less than zero, this means the boundary position is wrong
                const auto attenuation =
                    TMath::Exp(-distance / fVetoLightAttenuation);  // attenuation factor is in mm
                energy *= attenuation;
            }
            if (energy <= 0) {
                continue;
            }
            auto& signal = fVetoSignalMap.at(volume);
            const auto time = hits.GetTime(i) - triggerTime;
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
    SetVetoLightAttenuation(GetDblParameterWithUnits("vetoLightAttenuation", fVetoLightAttenuation));
    SetVetoQuenchingFactor(GetDblParameterWithUnits("quenchingFactor", fVetoQuenchingFactor));

    fDriftEnabled = StringToBool(GetParameter("drift", to_string(fDriftEnabled)));
    fDriftVolume = GetParameter("driftVolume", fDriftVolume);
    fDriftReadoutVolume = GetParameter("driftReadoutVolume", fDriftReadoutVolume);
    fDriftReadoutOffset = GetDblParameterWithUnits("driftReadoutOffset", fDriftReadoutOffset);
    fDriftReadoutNormalDirection =
        Get3DVectorParameterWithUnits("driftReadoutPlaneNormal", fDriftReadoutNormalDirection);
    fDriftVelocity = GetDblParameterWithUnits("driftVelocity", fDriftVelocity);
}

void TRestGeant4ToDetectorSignalVetoProcess::PrintMetadata() {
    BeginPrintProcess();

    cout << "Veto volume expression: " << fVetoVolumesExpression << endl;
    if (!fVetoDetectorsExpression.IsNull()) {
        cout << "Veto detector expression: " << fVetoDetectorsExpression << endl;
        cout << "Veto detector offset: " << fVetoDetectorOffsetSize << " mm" << endl;
        cout << "Veto light attenuation: " << fVetoLightAttenuation << " mm" << endl;
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

    if (fGeant4Metadata == nullptr) {
        return;
    }
    const auto& geometryInfo = fGeant4Metadata->GetGeant4GeometryInfo();
    for (int i = 0; i < fVetoVolumes.size(); i++) {
        const auto& vetoName = fVetoVolumes[i];
        const auto& vetoPosition = geometryInfo.GetPosition(vetoName);

        cout << TString::Format(" - Veto volume: %d - name: '%s' - position: %s mm\n", i, vetoName.Data(),
                                VectorToString(vetoPosition).c_str())
             << endl;

        if (fVetoDetectorVolumes.empty()) {
            continue;
        }

        const auto& vetoDetectorName = fVetoDetectorVolumes[i];
        const auto& vetoDetectorPosition = geometryInfo.GetPosition(vetoDetectorName);

        cout << TString::Format("   Veto detector name: '%s' - position: %s mm\n", vetoDetectorName.Data(),
                                VectorToString(vetoDetectorPosition).c_str());

        cout << TString::Format("   Boundary position: %s mm - direction: %s\n",
                                VectorToString(fVetoDetectorBoundaryPosition.at(vetoName)).c_str(),
                                VectorToString(fVetoDetectorBoundaryDirection.at(vetoName)).c_str());
    }

    if (!fDriftEnabled) {
        cout << "Drift is not enabled" << endl;
    } else {
        cout << "Drift is enabled" << endl;
        cout << " - Drift volume: " << fDriftVolume << endl;
        cout << " - Drift readout volume: " << fDriftReadoutVolume << endl;
        cout << " - Drift readout offset: " << fDriftReadoutOffset << " mm" << endl;
        cout << " - Drift readout normal: " << VectorToString(fDriftReadoutNormalDirection) << endl;
        cout << " - Drift velocity: " << fDriftVelocity << " mm/us" << endl;
    }

    EndPrintProcess();
}
