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

#include "TObjString.h"
#include "TPRegexp.h"

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
/// \param cfgFileName A const char* giving the path to an RML file.
///
TRestGeant4ToDetectorHitsProcess::TRestGeant4ToDetectorHitsProcess(const char* cfgFileName) {
    Initialize();

    if (LoadConfigFromFile(cfgFileName)) LoadDefaultConfig();
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
/// \param cfgFileName A const char* giving the path to an RML file.
/// \param name The name of the specific metadata. It will be used to find the
/// correspondig TRestGeant4ToDetectorHitsProcess section inside the RML.
///
void TRestGeant4ToDetectorHitsProcess::LoadConfig(std::string cfgFilename, std::string name) {
    if (LoadConfigFromFile(std::move(cfgFilename), std::move(name))) LoadDefaultConfig();
}

///////////////////////////////////////////////
/// \brief Process initialization. This process accesses the information inside
/// TRestGeant4Metadata to identify the geometry volume ids associated to the hits.
///
void TRestGeant4ToDetectorHitsProcess::InitProcess() {
    debug << "Active volumes available in TRestGeant4Metadata" << endl;
    debug << "-------------------------------------------" << endl;
    for (int n = 0; n < fGeant4Metadata->GetNumberOfActiveVolumes(); n++)
        debug << "Volume id : " << n << " name : " << fGeant4Metadata->GetActiveVolumeName(n) << endl;
    debug << endl;

    debug << "TRestGeant4HitsProcess volumes enabled in RML : ";
    debug << "-------------------------------------------" << endl;
    if (fVolumeNames.empty())
        debug << "all" << endl;
    else {
        for (auto& n : fVolumeNames) {
            debug << "" << endl;
            debug << " - " << n << endl;
        }
        debug << " " << endl;
    }

    if (!fVolumeNames.empty()) {
        debug << "TRestGeant4HitsProcess volumes identified : ";
        debug << "---------------------------------------" << endl;
        if (fVolumeNames.empty())
            debug << "all" << endl;
        else
            for (auto& n : fVolumeNames) {
                debug << "" << endl;
                debug << " - " << n << endl;
            }
        debug << " " << endl;
    }

    //
    for (const auto& physicalVolume : fGeant4Metadata->GetPhysicalVolumes()) {
        cout << "---> Volume: " << physicalVolume
             << " with logical: " << fGeant4Metadata->GetLogicalVolume(physicalVolume) << endl;
    }
}

void TRestGeant4ToDetectorHitsProcess::EndProcess() {
    return;

    info << "Number of detector hits: " << fHitsEvent->GetNumberOfHits() << endl;

    auto vetoHits = fHitsEvent->GetVetoHits();

    info << "Number of veto volumes: " << vetoHits.size() << endl;

    for (const auto& [volumeName, hits] : vetoHits) {
        info << "---> " << volumeName << ": " << hits->GetNumberOfHits() << " hits" << endl;
    }
}

///////////////////////////////////////////////
/// \brief The main processing event function
///
TRestEvent* TRestGeant4ToDetectorHitsProcess::ProcessEvent(TRestEvent* inputEvent) {
    fGeant4Event = (TRestGeant4Event*)inputEvent;

    if (this->GetVerboseLevel() >= REST_Extreme) {
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
        for (int i = 0; i < track.GetNumberOfHits(); i++) {
            const auto hits = track.GetHitsConst();
            TVector3 position = {hits.GetX(i), hits.GetY(i), hits.GetZ(i)};
            auto energy = hits.GetEnergy(i);
            auto time = hits.GetTime(i);

            if (energy <= 0) {
                continue;  // Do nothing with 0 energy hits
            }

            auto volumeName = hits.GetVolumeName(i);

            if (fVolumeNames.count(volumeName) > 0 || fVolumeNames.empty()) {
                fHitsEvent->AddHit(position, energy);
            }
            if (fVetoVolumeNames.count(volumeName) > 0) {
                /*
                cout << TString::Format("Hit at {%.2f, %.2f, %.2f} mm with %.2f keV", position.x(),
                                        position.y(), position.z(), energy)
                     << endl;
                */
                TVector3 interface = fLightGuideInterfacePosition[volumeName];
                TVector3 direction = fScintillatorToLightGuideDirection[volumeName];
                /*
                cout << TString::Format("Interface at {%.2f, %.2f, %.2f} mm", interface.x(), interface.y(),
                                        interface.z())
                     << endl;

                cout << TString::Format("Direction {%.2f, %.2f, %.2f}", direction.x(), direction.y(),
                                        direction.z())
                     << endl;
                */
                Double_t distance = interface.Dot(direction) - position.Dot(direction);
                Double_t length =
                    fScintillatorLogicalLength.at(fGeant4Metadata->GetLogicalVolume(volumeName));
                if (distance < 0 || distance > length) {
                    cout << "distance out of bounds, please check! "
                         << fGeant4Metadata->GetLogicalVolume(volumeName) << " - " << volumeName << " - "
                         << distance << " - " << length << endl;
                    exit(1);
                }

                Double_t attenuationLength =
                    fScintillatorLogicalToAttenuation.at(fGeant4Metadata->GetLogicalVolume(volumeName));

                Double_t attenuationFactor = TMath::Exp(-1.0 * distance / attenuationLength);
                /*
                cout << TString::Format("Distance of %.2f mm, attenuation length of %.2f, factor of %10.10f",
                                        distance, attenuationLength, attenuationFactor)
                     << endl;
                cout << endl;
                */
                // We move the hit to the interface position and adjust its energy for attenuation
                fHitsEvent->AddVetoHit(volumeName, interface, energy * attenuationFactor, time);
            }
        }
    }

    if (this->GetVerboseLevel() >= REST_Debug) {
        cout << "TRestGeant4ToDetectorHitsProcess. Hits added : " << fHitsEvent->GetNumberOfHits() << endl;
        cout << "TRestGeant4ToDetectorHitsProcess. Hits total energy : " << fHitsEvent->GetEnergy() << endl;
    }

    return fHitsEvent;
}

tuple<TString, TString, TString> GetAssemblyInfoFromPhysicalVolume(const TString& physicalVolume,
                                                                   const TString& logicalVolume) {
    // parse input such as av_24_impr_11_scintillatorLightGuideVolume-1500.0mm_pv_4
    std::tuple<TString, TString, TString> assemblyInfo;

    TString aux = TString::Format(R"(\bav_(\d+)_impr_(\d+)_%s_pv_(\d+)\b)", logicalVolume.Data());
    TPRegexp regex(aux);  // https://root.cern.ch/doc/v608/regexp_8C.html
    TObjArray* subStrL = regex.MatchS(physicalVolume);
    if (subStrL->GetLast() != 3) {
        cout << "error matching regex! check input parameters" << endl;
        exit(1);
    }

    const TString av = ((TObjString*)subStrL->At(1))->GetString();
    const TString impr = ((TObjString*)subStrL->At(2))->GetString();
    const TString pv = ((TObjString*)subStrL->At(3))->GetString();

    std::get<0>(assemblyInfo) = av;
    std::get<1>(assemblyInfo) = impr;
    std::get<2>(assemblyInfo) = pv;

    return assemblyInfo;
}

///////////////////////////////////////////////
/// \brief Function to read input parameters from the RML
/// TRestGeant4ToDetectorHitsProcess metadata section
///
void TRestGeant4ToDetectorHitsProcess::InitFromConfigFile() {
    size_t position = 0;
    string definition;

    while (!(definition = GetKEYDefinition("addVolume", position)).empty()) {
        fVolumeUserSelection.insert(GetFieldValue("name", definition));
    }

    while (!(definition = GetKEYDefinition("addVeto", position)).empty()) {
        TString scintillatorLogical = GetFieldValue("scintillatorLogical", definition);
        TString lightGuideLogical = GetFieldValue("lightGuideLogical", definition);
        Double_t distanceToLightGuide = StringToDouble(GetFieldValue("length", definition));  // In mm
        Double_t attenuationDistance = StringToDouble(
            GetFieldValue("attenuation", definition));  // Attenuation length in mm (decrease 1/e)

        fScintillatorLogicalNames.insert(scintillatorLogical);
        fScintillatorLogicalToLightGuideLogical[scintillatorLogical] = lightGuideLogical;
        fScintillatorLogicalLength[scintillatorLogical] = distanceToLightGuide;
        fScintillatorLogicalToAttenuation[scintillatorLogical] = attenuationDistance;
    }

    fGeant4Metadata = GetMetadata<TRestGeant4Metadata>();

    for (const auto& volumeName : fVolumeUserSelection) {
        if (fGeant4Metadata->IsValidVolumeName(volumeName))
            fVolumeNames.insert(volumeName);
        else {
            // maybe the user specified the logical volume name as unique id
            auto volumeNameFromLogical = fGeant4Metadata->GetUniquePhysicalVolumeFromLogical(volumeName);
            if (!fGeant4Metadata->IsValidVolumeName(volumeNameFromLogical)) {
                cout << "TRestGeant4ToDetectorHitsProcess - Volume name: " << volumeName
                     << " not found and will not be added." << endl;
                exit(1);
            } else {
                cout << "Added physical volume '" << volumeNameFromLogical << "' from logical '" << volumeName
                     << "'" << endl;
                fVolumeNames.insert(volumeNameFromLogical);
            }
        }
    }

    for (const auto& scintillatorLogicalName : fScintillatorLogicalNames) {
        // maybe the user specified the logical volume name ID (not required to be unique!)
        cout << "- Processing logical: " << scintillatorLogicalName << endl;
        auto volumeNamesFromLogical =
            fGeant4Metadata->GetAllPhysicalVolumeFromLogical(scintillatorLogicalName);
        if (volumeNamesFromLogical.empty()) {
            cout << "TRestGeant4ToDetectorHitsProcess - Volume name: " << scintillatorLogicalName
                 << " not found and will not be added." << endl;
            exit(1);
        }
        for (const auto& volumeNameFromLogical : volumeNamesFromLogical) {
            cout << "Added physical volume '" << volumeNameFromLogical << "' from logical '"
                 << scintillatorLogicalName << "'" << endl;
            fVetoVolumeNames.insert(volumeNameFromLogical);
        }

        auto lightGuideLogical = fScintillatorLogicalToLightGuideLogical.at(scintillatorLogicalName);
        cout << "Light guide logical: " << lightGuideLogical << endl;
        // there has to be one light guide per scintillator
        auto lightGuideVolumeNamesFromLogical =
            fGeant4Metadata->GetAllPhysicalVolumeFromLogical(lightGuideLogical);
        if (lightGuideVolumeNamesFromLogical.size() != volumeNamesFromLogical.size()) {
            cout << "light guide volumes do not match scintillator volumes! ("
                 << lightGuideVolumeNamesFromLogical.size() << " to " << volumeNamesFromLogical.size() << ")"
                 << endl;
            cout << "Probably some logical volume name is misspelled, please check with all logical volume "
                    "names below:"
                 << endl;
            for (const auto& name : fGeant4Metadata->GetLogicalVolumes()) {
                cout << "---> " << name << endl;
            }
            exit(1);
        }
        // We need to match each scintillator physical volume to a light guide physical

        for (const auto& scintillatorPhysical : volumeNamesFromLogical) {
            size_t matches = 0;

            auto assemblyInfo =
                GetAssemblyInfoFromPhysicalVolume(scintillatorPhysical, scintillatorLogicalName);
            TString av = get<0>(assemblyInfo);
            TString impr = get<1>(assemblyInfo);

            for (const auto& lightGuidePhysical : lightGuideVolumeNamesFromLogical) {
                auto _assemblyInfo = GetAssemblyInfoFromPhysicalVolume(lightGuidePhysical, lightGuideLogical);
                TString _av = get<0>(_assemblyInfo);
                TString _impr = get<1>(_assemblyInfo);

                if (av == _av && impr == _impr) {
                    matches += 1;

                    // cout << scintillatorPhysical << " to " << lightGuidePhysical << endl;
                    fScintillatorPhysicalToLightGuidePhysicalMap[scintillatorPhysical] = lightGuidePhysical;
                }
            }

            if (matches != 1) {
                cout << "error matching scintillator to light guide" << endl;
                exit(1);
            }

            // Compute distance:
            auto lightGuidePhysical = fScintillatorPhysicalToLightGuidePhysicalMap.at(scintillatorPhysical);
            auto distance = fGeant4Metadata->GetPhysicalVolumePosition(lightGuidePhysical) -
                            fGeant4Metadata->GetPhysicalVolumePosition(scintillatorPhysical);

            fScintillatorToLightGuideDirection[scintillatorPhysical] = distance.Unit();
            fScintillatorPosition[scintillatorPhysical] =
                fGeant4Metadata->GetPhysicalVolumePosition(scintillatorPhysical);
            fLightGuideInterfacePosition[scintillatorPhysical] =
                fScintillatorPosition.at(scintillatorPhysical) +
                fScintillatorToLightGuideDirection.at(scintillatorPhysical) *
                    fScintillatorLogicalLength.at(fGeant4Metadata->GetLogicalVolume(scintillatorPhysical)) *
                    0.5;

            /*
            cout << TString::Format("Distance from '%s' to '%s' is {%.2f, %.2f, %.2f} mm",
                                    scintillatorPhysical.Data(), lightGuidePhysical.Data(), distance.x(),
                                    distance.y(), distance.z())
                 << endl;
            */
        }
    }
}

///////////////////////////////////////////////
/// \brief It prints on screen relevant data members from this class
///
void TRestGeant4ToDetectorHitsProcess::PrintMetadata() {
    BeginPrintProcess();

    for (auto& volume : fVolumeNames) {
        metadata << "Volume added: " << volume << endl;
    }

    metadata << endl;

    for (auto& volume : fVetoVolumeNames) {
        metadata << "Veto Volume added: " << volume << endl;
    }

    EndPrintProcess();
}

#include <TEveGeoNode.h>
#include <TEveManager.h>
#include <TEvePointSet.h>
#include <TGLViewer.h>
#include <TGeoManager.h>

void TRestGeant4ToDetectorHitsProcess::DrawGeometryVetoPosition() {
    // this should be run from the root prompt
    auto geo = gGeoManager;
    if (!geo) {
        metadata << "No TGeoManager global object, maybe geometry is not saved to file? Please, load it from "
                    "TRestGeant4Metadata first with 'TRestGeant4Metadata::LoadGeometry()' method"
                 << endl;
        auto geant4Metadata = GetMetadata<TRestGeant4Metadata>();
        if (geant4Metadata) {
            geant4Metadata->LoadGeometry();
            return;
        } else {
            cout << "Could not load Geant4 metadata" << endl;
        }
    }

    if (!geo) {
        metadata << "TGeoManager could not be loaded" << endl;
        return;
    }

    auto eve = TEveManager::Create();

    auto topNode = new TEveGeoTopNode(gGeoManager, gGeoManager->GetTopNode());
    topNode->SetVisLevel(5);

    eve->AddGlobalElement(topNode);

    // Transparency
    const int transparency = 75;
    for (int i = 0; i < gGeoManager->GetListOfVolumes()->GetEntries(); i++) {
        auto volume = gGeoManager->GetVolume(i);
        auto material = volume->GetMaterial();
        if (material->GetDensity() <= 0.01) {
            volume->SetTransparency(95);
            if (material->GetDensity() <= 0.001) {
                // We consider this vacuum for display purposes
                volume->SetVisibility(kFALSE);
            }
        } else {
            volume->SetTransparency(transparency);
        }
    }

    std::map<TString, TEvePointSet*> pointSetMap;
    for (const auto& [name, position] : fLightGuideInterfacePosition) {
        auto ps = new TEvePointSet(name);
        pointSetMap[name] = ps;
        ps->SetNextPoint(position.x(), position.y(), position.z());
        ps->SetMarkerStyle(10);
        ps->SetMarkerColor(kYellow);
        eve->AddElement(ps);
    }

    // Draw interfaces as points

    eve->FullRedraw3D(kTRUE);

    auto viewer = eve->GetDefaultGLViewer();
    // viewer->GetClipSet()->SetClipType(TGLClip::EType(2));
    viewer->CurrentCamera().Reset();
    // viewer->SetCurrentCamera(TGLViewer::kCameraOrthoZOY);
    //  viewer->SetCurrentCamera(TGLViewer::kCameraPerspXOZ);
    viewer->DoDraw();
}
