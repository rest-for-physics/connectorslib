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

#ifndef RestCore_TRestGeant4ToDetectorHitsProcess
#define RestCore_TRestGeant4ToDetectorHitsProcess

#include <TRestDetectorHitsEvent.h>
#include <TRestGeant4Event.h>
#include <TRestGeant4Metadata.h>

#include "TRestEventProcess.h"

/// A process to transform a *TRestGeant4Event* into a *TRestDetectorHitsEvent*.
class TRestGeant4ToDetectorHitsProcess : public TRestEventProcess {
   private:
    /// A pointer to the input TRestGeant4Event
    TRestGeant4Event* fGeant4Event;  //!

    /// A pointer to the Geant4 simulation conditions stored in TRestGeant4Metadata
    TRestGeant4Metadata* fGeant4Metadata;  //!

    /// A pointer to the output TRestDetectorHitsEvent
    TRestDetectorHitsEvent* fHitsEvent;  //!

    /// The volume ids from the volumes selected for transfer to TRestDetectorHitsEvent
    std::set<TString> fVolumeNames;

    // Parameters related to vetos

    std::set<TString> fVetoVolumeNames;

    std::set<TString> fVolumeUserSelection;  //!

    std::set<TString> fScintillatorLogicalNames;  //!

    std::map<TString, TString> fScintillatorLogicalToLightGuideLogical;  //!
    std::map<TString, Double_t> fScintillatorLogicalToAttenuation;       //!
    std::map<TString, Double_t> fScintillatorLogicalLength;

    std::map<TString, TString> fScintillatorPhysicalToLightGuidePhysicalMap;  //!

    std::map<TString, TVector3> fScintillatorPosition;
    std::map<TString, TVector3> fLightGuideInterfacePosition;
    std::map<TString, TVector3> fScintillatorToLightGuideDirection;

    void InitFromConfigFile();

    void Initialize();

    void LoadDefaultConfig();

   protected:
    // add here the members of your event process

   public:
    any GetInputEvent() { return fGeant4Event; }
    any GetOutputEvent() { return fHitsEvent; }

    void InitProcess();
    void EndProcess() override;

    TRestEvent* ProcessEvent(TRestEvent* inputEvent);

    void LoadConfig(std::string cfgFilename, std::string name = "");

    void PrintMetadata();

    /// Returns the name of this process
    TString GetProcessName() { return (TString) "g4toHitsEvent"; }

    void DrawGeometryVetoPosition();

    // Constructor
    TRestGeant4ToDetectorHitsProcess();
    TRestGeant4ToDetectorHitsProcess(const char* cfgFileName);
    // Destructor
    ~TRestGeant4ToDetectorHitsProcess();

    ClassDef(TRestGeant4ToDetectorHitsProcess, 1);  // Transform a TRestGeant4Event event to a
                                                    // TRestDetectorHitsEvent (hits-collection event)
};
#endif
