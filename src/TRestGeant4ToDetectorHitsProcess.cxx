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
/// using the `<addVolume` key inside the process definition.
///
/// The following example shows how to include the process into
/// `TRestProcessRunner` RML definition. In this particular example we
/// extract hits from `gas` and `vessel` volumes defined in the geometry.
/// Any other hits will be ignored.
///
/// \code
///
/// <addProcess type="TRestGeant4ToDetectorHitsProcess" name="g4ToHits" value="ON">
///     <addVolume name="gas" />
///     <addVolume name="vessel" />
/// </addProcess>
/// \endcode
///
/// If no volumes are defined using the `<addVolume` key, **all volumes will
/// be active**, and all hits will be transferred to the TRestDetectorHitsEvent output.
///
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

    if (LoadConfigFromFile(configFilename)) LoadDefaultConfig();
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

    cout << "G4 to hits metadata not found. Loading default values" << endl;
}

///////////////////////////////////////////////
/// \brief Function to initialize input/output event members and define the
/// section name
///
void TRestGeant4ToDetectorHitsProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fG4Event = nullptr;
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
    fG4Metadata = GetMetadata<TRestGeant4Metadata>();

    for (unsigned int n = 0; n < fVolumeSelection.size(); n++) {
        if (fG4Metadata->GetActiveVolumeID(fVolumeSelection[n]) >= 0)
            fVolumeId.push_back(fG4Metadata->GetActiveVolumeID(fVolumeSelection[n]));
        else if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Warning)
            cout << "TRestGeant4ToDetectorHitsProcess. volume name : " << fVolumeSelection[n]
                 << " not found and will not be added." << endl;
    }

    /* {{{ Debug output */
    RESTDebug << "Active volumes available in TRestGeant4Metadata" << RESTendl;
    RESTDebug << "-------------------------------------------" << RESTendl;
    for (unsigned int n = 0; n < fG4Metadata->GetNumberOfActiveVolumes(); n++)
        RESTDebug << "Volume id : " << n << " name : " << fG4Metadata->GetActiveVolumeName(n) << RESTendl;
    RESTDebug << RESTendl;

    RESTDebug << "TRestGeant4HitsProcess volumes enabled in RML : ";
    RESTDebug << "-------------------------------------------" << RESTendl;
    if (fVolumeSelection.size() == 0)
        RESTDebug << "all" << RESTendl;
    else {
        for (unsigned int n = 0; n < fVolumeSelection.size(); n++) {
            RESTDebug << "" << RESTendl;
            RESTDebug << " - " << fVolumeSelection[n] << RESTendl;
        }
        RESTDebug << " " << RESTendl;
    }

    if (fVolumeSelection.size() > 0 && fVolumeSelection.size() != fVolumeId.size())
        RESTWarning << "TRestGeant4ToDetectorHitsProcess. Not all volumes were properly identified!"
                    << RESTendl;

    if (fVolumeId.size() > 0) {
        RESTDebug << "TRestGeant4HitsProcess volumes identified : ";
        RESTDebug << "---------------------------------------" << RESTendl;
        if (fVolumeSelection.size() == 0)
            RESTDebug << "all" << RESTendl;
        else
            for (unsigned int n = 0; n < fVolumeSelection.size(); n++) {
                RESTDebug << "" << RESTendl;
                RESTDebug << " - " << fVolumeSelection[n] << RESTendl;
            }
        RESTDebug << " " << RESTendl;
    }
    /* }}} */
}

///////////////////////////////////////////////
/// \brief The main processing event function
///
TRestEvent* TRestGeant4ToDetectorHitsProcess::ProcessEvent(TRestEvent* inputEvent) {
    fG4Event = (TRestGeant4Event*)inputEvent;

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Extreme) {
        cout << "------ TRestGeant4ToDetectorHitsProcess --- Printing Input Event --- START ----" << endl;
        fG4Event->PrintEvent();
        cout << "------ TRestGeant4ToDetectorHitsProcess --- Printing Input Event ---- END ----" << endl;
        GetChar();
    }

    fHitsEvent->SetRunOrigin(fG4Event->GetRunOrigin());
    fHitsEvent->SetSubRunOrigin(fG4Event->GetSubRunOrigin());
    fHitsEvent->SetID(fG4Event->GetID());
    fHitsEvent->SetSubID(fG4Event->GetSubID());
    fHitsEvent->SetSubEventTag(fG4Event->GetSubEventTag());
    fHitsEvent->SetTimeStamp(fG4Event->GetTimeStamp());
    fHitsEvent->SetState(fG4Event->isOk());

    for (unsigned int i = 0; i < fG4Event->GetNumberOfTracks(); i++) {
        const auto& track = fG4Event->GetTrack(i);
        const auto& hits = track.GetHits();
        for (unsigned int j = 0; j < track.GetNumberOfHits(); j++) {
            const auto energy = hits.GetEnergy(j);
            for (const auto& volumeID : fVolumeId) {
                if (hits.GetVolumeId(j) == volumeID && energy > 0) {
                    fHitsEvent->AddHit(hits.GetX(j), hits.GetY(j), hits.GetZ(j), energy);
                }
            }
            if (fVolumeId.empty() && energy > 0) {
                fHitsEvent->AddHit(hits.GetX(j), hits.GetY(j), hits.GetZ(j), energy);
            }
        }
    }

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug) {
        cout << "TRestGeant4ToDetectorHitsProcess. Hits added : " << fHitsEvent->GetNumberOfHits() << endl;
        cout << "TRestGeant4ToDetectorHitsProcess. Hits total energy : " << fHitsEvent->GetEnergy() << endl;
    }

    return fHitsEvent;
}

///////////////////////////////////////////////
/// \brief Function to read input parameters from the RML
/// TRestGeant4ToDetectorHitsProcess metadata section
///
void TRestGeant4ToDetectorHitsProcess::InitFromConfigFile() {
    size_t position = 0;
    string addVolumeDefinition;

    while ((addVolumeDefinition = GetKEYDefinition("addVolume", position)) != "")
        fVolumeSelection.push_back(GetFieldValue("name", addVolumeDefinition));
}

///////////////////////////////////////////////
/// \brief It prints on screen relevant data members from this class
///
void TRestGeant4ToDetectorHitsProcess::PrintMetadata() {
    BeginPrintProcess();

    for (unsigned int n = 0; n < fVolumeSelection.size(); n++)
        RESTMetadata << "Volume added : " << fVolumeSelection[n] << RESTendl;

    EndPrintProcess();
}
