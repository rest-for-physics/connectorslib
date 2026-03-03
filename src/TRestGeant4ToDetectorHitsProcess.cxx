/*************************************************************************
 * This file is part of the REST software framework.                     *
 *                                                                       *
 * Copyright (C) 2016 GIFNA/TREX (University of Zaragoza)                *
 * For more information see http://gifna.unizar.es/trex                  *
 *                                                                       *
 * REST is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * REST is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have a copy of the GNU General Public License along with   *
 * REST in $REST_PATH/LICENSE.                                           *
 * If not, see http://www.gnu.org/licenses/.                             *
 * For the list of contributors see $REST_PATH/CREDITS.                  *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
/// This process allows to select the GDML geometry volumes (defined in
/// TRestGeant4Metadata) that will be transferred to the TRestDetectorHitsEvent by
/// using the `<volume` key inside the process definition.
///
/// ## Basic usage
/// The following example shows how to include the process into
/// `TRestProcessRunner` RML definition. In this particular example we
/// extract hits from `gas` and `vessel` volumes defined in the geometry.
/// Any other hits will be ignored.
///
/// \code
/// <addProcess type="TRestGeant4ToDetectorHitsProcess" name="g4ToHits" value="ON">
///     <volume name="gas"/>
///     <volume name="vessel"/>
/// </addProcess>
/// \endcode
///
/// If no volumes are defined using the `<volume` key, **all volumes will
/// be active**, and all hits will be transferred to the TRestDetectorHitsEvent output.
///
/// ## Advanced options
/// There are a couple of additional parameters that can be defined for each volume:
/// * **type**: The hit type associated to the selected volume. Useful for veto volumes by setting it to `veto`. Otherwise, the default value is `XYZ`.
/// * **gain**: A gain factor applied to the energy deposition of each hit in the selected volume. Default is 1.
///
/// For example:
/// \code
/// <addProcess type="TRestGeant4ToDetectorHitsProcess" name="g4ToHits" value="ON">
///     <volume name="driftGas"/> <!-- default hit type is XYZ and gain is 1 -->
///     <volume name="transferGas" gain="0.1"/>
///     <volume name="scintillator" type="veto"/>
/// </addProcess>
/// \endcode
///--------------------------------------------------------------------------
///
/// RESTsoft - Software for Rare Event Searches with TPCs
///
/// History of developments:
///
/// 2016-October First implementation of TRestGeant4Event to TRestDetectorHitsEvent
///              Igor Irastorza
///
/// 2017-October: Added the possibility to extract hits only from selected geometrical volumes
///               Javier Galan
///
/// \class      TRestGeant4ToDetectorHitsProcess
/// \author     Igor Irastorza
/// \author     Javier Galan
///
/// <hr>
///
#include "TRestGeant4ToDetectorHitsProcess.h"

using namespace std;

ClassImp(TRestGeant4ToDetectorHitsProcess);

///////////////////////////////////////////////
/// \brief Default constructor
///
TRestGeant4ToDetectorHitsProcess::TRestGeant4ToDetectorHitsProcess() { Initialize(); }

///////////////////////////////////////////////
/// \brief Constructor loading data from a config file
///
/// If no configuration path is defined using TRestMetadata::SetConfigFilePath
/// the path to the config file must be specified using full path, absolute or
/// relative.
///
/// The default behaviour is that the config file must be specified with
/// full path, absolute or relative.
///
/// \param configFilename A const char* giving the path to an RML file.
///
TRestGeant4ToDetectorHitsProcess::TRestGeant4ToDetectorHitsProcess(const char* configFilename) {
    Initialize();

    if (LoadConfigFromFile(configFilename)) {
        LoadDefaultConfig();
    }
}

///////////////////////////////////////////////
/// \brief Default destructor
///
TRestGeant4ToDetectorHitsProcess::~TRestGeant4ToDetectorHitsProcess() { delete fHitsEvent; }

///////////////////////////////////////////////
/// \brief Function to load the default config in absence of RML input
///
void TRestGeant4ToDetectorHitsProcess::LoadDefaultConfig() {
    SetTitle("Default config");

    cout << "Geant4 to hits metadata not found. Loading default values" << endl;
}

///////////////////////////////////////////////
/// \brief Function to initialize input/output event members and define the
/// section name
///
void TRestGeant4ToDetectorHitsProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fGeant4Event = nullptr;
    fHitsEvent = new TRestDetectorHitsEvent();
}

///////////////////////////////////////////////
/// \brief Function to load the configuration from an external configuration
/// file.
///
/// If no configuration path is defined in TRestMetadata::SetConfigFilePath
/// the path to the config file must be specified using full path, absolute or
/// relative.
///
/// \param configFilename A const char* giving the path to an RML file.
/// \param name The name of the specific metadata. It will be used to find the
/// corresponding TRestGeant4ToDetectorHitsProcess section inside the RML.
///
void TRestGeant4ToDetectorHitsProcess::LoadConfig(const string& configFilename, const string& name) {
    if (LoadConfigFromFile(configFilename, name)) LoadDefaultConfig();
}

///////////////////////////////////////////////
/// \brief Process initialization. This process accesses the information inside
/// TRestGeant4Metadata to identify the geometry volume ids associated to the hits.
///
void TRestGeant4ToDetectorHitsProcess::InitProcess() {
    fGeant4Metadata = GetMetadata<TRestGeant4Metadata>();

    for (const auto& userVolume : fVolumeSelection) {
        auto volId = fGeant4Metadata->GetActiveVolumeID(userVolume);
        if (volId >= 0) {
            VolumeProperties properties{volId, userVolume, REST_HitType::XYZ, 1.0};  // default values
            for (size_t i = 0; i < fVolumeSelection.size(); i++) {
                if (fVolumeSelection[i] == userVolume) {
                    properties.hitType = fVolumeHitType[i];
                    properties.gain = fVolumeGain[i];
                    break;
                }
            }
            fVolumeProperties.push_back(properties);
        } else if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Warning)
            cout << "TRestGeant4ToDetectorHitsProcess. volume name : " << userVolume
                 << " not found and will not be added." << endl;
    }

    // sort fVolumeProperties by volumeID for faster access when processing the event
    sort(fVolumeProperties.begin(), fVolumeProperties.end(),
         [](const VolumeProperties& a, const VolumeProperties& b) { return a.volumeID < b.volumeID; });
    // erase duplicate volumeIDs in fVolumeProperties (if any)
    fVolumeProperties.erase(
        unique(fVolumeProperties.begin(), fVolumeProperties.end(),
               [](const VolumeProperties& a, const VolumeProperties& b) { return a.volumeID == b.volumeID; }),
        fVolumeProperties.end());

    for (size_t i = 0; i < fVolumeProperties.size(); i++) {
        RESTDebug << "TRestGeant4ToDetectorHitsProcess. Volume id : " << fVolumeProperties[i].volumeID
                  << " name : " << fGeant4Metadata->GetActiveVolumeName(fVolumeProperties[i].volumeID)
                  << RESTendl;
    }

    RESTDebug << "Active volumes available in TRestGeant4Metadata" << RESTendl;
    RESTDebug << "-------------------------------------------" << RESTendl;
    for (unsigned int n = 0; n < fGeant4Metadata->GetNumberOfActiveVolumes(); n++) {
        RESTDebug << "Volume id : " << n << " name : " << fGeant4Metadata->GetActiveVolumeName(n) << RESTendl;
    }
    RESTDebug << RESTendl;

    RESTDebug << "TRestGeant4HitsProcess volumes enabled in RML : ";
    RESTDebug << "-------------------------------------------" << RESTendl;
    if (fVolumeSelection.empty())
        RESTDebug << "all" << RESTendl;
    else {
        for (const auto& volume : fVolumeSelection) {
            RESTDebug << "" << RESTendl;
            RESTDebug << " - " << volume << RESTendl;
        }
        RESTDebug << " " << RESTendl;
    }

    if (!fVolumeSelection.empty() && fVolumeSelection.size() != fVolumeProperties.size())
        RESTWarning << "TRestGeant4ToDetectorHitsProcess. Not all volumes were properly identified!"
                    << RESTendl;

    if (!fVolumeProperties.empty()) {
        RESTDebug << "TRestGeant4HitsProcess volumes identified : ";
        RESTDebug << "---------------------------------------" << RESTendl;
        if (fVolumeSelection.empty())
            RESTDebug << "all" << RESTendl;
        else
            for (const auto& volume : fVolumeSelection) {
                RESTDebug << "" << RESTendl;
                RESTDebug << " - " << volume << RESTendl;
            }
        RESTDebug << " " << RESTendl;
    }
}

///////////////////////////////////////////////
/// \brief The main processing event function
///
TRestEvent* TRestGeant4ToDetectorHitsProcess::ProcessEvent(TRestEvent* inputEvent) {
    fGeant4Event = (TRestGeant4Event*)inputEvent;

    fGeant4Event->InitializeReferences(GetRunInfo());

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Extreme) {
        cout << "------ TRestGeant4ToDetectorHitsProcess --- Printing Input Event --- START ----" << endl;
        fGeant4Event->PrintEvent();
        cout << "------ TRestGeant4ToDetectorHitsProcess --- Printing Input Event ---- END ----" << endl;
        GetChar();
    }

    fHitsEvent->SetRunOrigin(fGeant4Event->GetRunOrigin());
    fHitsEvent->SetSubRunOrigin(fGeant4Event->GetSubRunOrigin());
    fHitsEvent->SetID(fGeant4Event->GetID());
    fHitsEvent->SetSubID(fGeant4Event->GetSubID());
    fHitsEvent->SetSubEventTag(fGeant4Event->GetSubEventTag());
    fHitsEvent->SetTimeStamp(fGeant4Event->GetTimeStamp());
    fHitsEvent->SetState(fGeant4Event->isOk());

    for (const auto& track : fGeant4Event->GetTracks()) {
        const auto& hits = track.GetHits();
        for (unsigned int i = 0; i < track.GetNumberOfHits(); i++) {
            const auto energy = hits.GetEnergy(i);
            if (energy <= 0) {
                continue;
            }
            const TVector3& position = hits.GetPosition(i);
            const double time = hits.GetTime(i);

            if (time < 0) {
                cerr << "TRestGeant4ToDetectorHitsProcess. Negative time found. This should never happen"
                     << endl;
                exit(1);
            }
            if (fVolumeProperties.empty()) {
                // if no volume is selected, all hits are added
                fHitsEvent->AddHit(position.X(), position.Y(), position.Z(), energy, time);
            } else {
                const TString volumeName = hits.GetVolumeName(i);
                const auto volumeId = fGeant4Metadata->GetActiveVolumeID(volumeName);

                // cout << "volumeName : " << volumeName << " volumeId : " << volumeId << endl;
                auto it = find_if(fVolumeProperties.begin(), fVolumeProperties.end(),
                                  [volumeId](const VolumeProperties& vp) { return vp.volumeID == volumeId; });
                if (it != fVolumeProperties.end()) {
                    const auto& properties = *it;
                    const REST_HitType type = properties.hitType;
                    const Double_t gain = properties.gain;

                    fHitsEvent->AddHit(position, energy * gain, time, type);
                }
            }
        }
    }

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug) {
        cout << "TRestGeant4ToDetectorHitsProcess. Hits added : " << fHitsEvent->GetNumberOfHits() << endl;
        cout << "TRestGeant4ToDetectorHitsProcess. Hits total energy : " << fHitsEvent->GetTotalEnergy()
             << endl;
    }

    return fHitsEvent;
}

///////////////////////////////////////////////
/// \brief Function to read input parameters from the RML
/// TRestGeant4ToDetectorHitsProcess metadata section
///
void TRestGeant4ToDetectorHitsProcess::InitFromConfigFile() {
    // Attempt to access TRestGeant4Metadata
    fGeant4Metadata = GetMetadata<TRestGeant4Metadata>();
    if (fGeant4Metadata == nullptr) {
        RESTWarning << "TRestGeant4ToDetectorHitsProcess. No TRestGeant4Metadata found in the input file"
                    << RESTendl;
    }

    set<string> volumesToAdd;
    map<string, Double_t> volumeGain;
    map<string, REST_HitType> volumeHitType;
    TiXmlElement* volumeDefinition = GetElement("volume");
    if (volumeDefinition == nullptr) {
        volumeDefinition = GetElement("addVolume");
        if (volumeDefinition != nullptr) {
            RESTWarning << "TRestGeant4ToDetectorHitsProcess. 'addVolume' tag is deprecated. Please use "
                           "'volume' instead."
                        << RESTendl;
        }
    }
    while (volumeDefinition != nullptr) {
        const auto userVolume = GetFieldValue("name", volumeDefinition);
        const auto typeName = GetFieldValue("type", volumeDefinition);
        const Double_t gain = StringToDouble(GetParameter("gain", volumeDefinition, "1.0"));
        REST_HitType type = XYZ;
        if (typeName == "veto") {
            type = VETO;
        }

        if (userVolume == "Not defined") {
            RESTError << "TRestGeant4ToDetectorHitsProcess. No name defined for volume" << RESTendl;
        }
        if (fGeant4Metadata != nullptr) {
            const auto& geometryInfo = fGeant4Metadata->GetGeant4GeometryInfo();

            auto physicalVolumes = geometryInfo.GetAllPhysicalVolumesMatchingExpression(userVolume);
            if (physicalVolumes.empty()) {
                const auto logicalVolumes = geometryInfo.GetAllLogicalVolumesMatchingExpression(userVolume);
                for (const auto& logicalVolume : logicalVolumes) {
                    for (const auto& physicalVolume :
                         geometryInfo.GetAllPhysicalVolumesFromLogical(logicalVolume)) {
                        physicalVolumes.push_back(
                            geometryInfo.GetAlternativeNameFromGeant4PhysicalName(physicalVolume));
                    }
                }
            }
            for (const auto& physicalVolume : physicalVolumes) {
                volumesToAdd.insert(physicalVolume.Data());
                volumeHitType[physicalVolume.Data()] = type;
                volumeGain[physicalVolume.Data()] = gain;
            }
        } else {
            volumesToAdd.insert(userVolume);
            volumeHitType[userVolume] = type;
            volumeGain[userVolume] = gain;
        }

        volumeDefinition = GetNextElement(volumeDefinition);
    }

    for (const auto& volume : volumesToAdd) {
        if (find(fVolumeSelection.begin(), fVolumeSelection.end(), volume) == fVolumeSelection.end()) {
            fVolumeSelection.emplace_back(volume);

            // hit type should be in volumeHitType. Anyways, set default hit type of XYZ if not found
            if (volumeHitType.find(volume) != volumeHitType.end()) {
                fVolumeHitType.push_back(volumeHitType[volume]);
            } else {
                RESTWarning << "TRestGeant4ToDetectorHitsProcess. No hit type defined for volume " << volume
                            << ". Using default hit type XYZ" << RESTendl;
                fVolumeHitType.push_back(XYZ);
            }

            // volume should be in volumeGain. Anyways, set deafult gain of 1.0 if not found
            if (volumeGain.find(volume) != volumeGain.end()) {
                fVolumeGain.push_back(volumeGain[volume]);
            } else {
                RESTWarning << "TRestGeant4ToDetectorHitsProcess. No gain defined for volume " << volume
                            << ". Using default gain of 1.0" << RESTendl;
                fVolumeGain.push_back(1.0);
            }
        }
    }
}

///////////////////////////////////////////////
/// \brief It prints on screen relevant data members from this class
///
void TRestGeant4ToDetectorHitsProcess::PrintMetadata() {
    BeginPrintProcess();

    for (size_t i = 0; i < fVolumeSelection.size(); i++) {
        const auto& volume = fVolumeSelection[i];
        const auto& gain = fVolumeGain[i];
        const auto& hitType = fVolumeHitType[i];
        RESTMetadata << "Volume added : " << volume << " with gain : " << gain
                     << " and hit type : " << hitType << RESTendl;
    }

    EndPrintProcess();
}
