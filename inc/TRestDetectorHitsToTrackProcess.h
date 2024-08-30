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

#ifndef RestCore_TRestDetectorHitsToTrackProcess
#define RestCore_TRestDetectorHitsToTrackProcess

#include <TRestDetectorHitsEvent.h>
#include <TRestTrackEvent.h>

#include "TMatrixD.h"
#include "TRestEventProcess.h"

//! A process to convert a TRestDetectorHitsEvent into a TRestTrackEvent
class TRestDetectorHitsToTrackProcess : public TRestEventProcess {
   private:
#ifndef __CINT__
    TRestDetectorHitsEvent* fHitsEvent;  //!
    TRestTrackEvent* fTrackEvent;        //!
#endif

    void Initialize() override;
    Int_t FindTracks(TRestHits* hits);

   protected:
    /// The hits distance used to define a cluster of hits
    Double_t fClusterDistance = 2.5;
    Bool_t fIgnoreOneHitTracks = false;

   public:
    RESTValue GetInputEvent() const override { return fHitsEvent; }
    RESTValue GetOutputEvent() const override { return fTrackEvent; }

    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;

    /// It prints out the process parameters stored in the metadata structure
    void PrintMetadata() override {
        BeginPrintProcess();

        RESTMetadata << " cluster-distance : " << fClusterDistance << " mm " << RESTendl;
        RESTMetadata << " ignoreOneHitTracks : " << fIgnoreOneHitTracks << " 0=false, 1=true " << RESTendl;

        EndPrintProcess();
    }

    /// Returns the name of this process
    const char* GetProcessName() const override { return "hitsToTrack"; }

    void InitFromConfigFile() override;

    TRestDetectorHitsToTrackProcess();
    ~TRestDetectorHitsToTrackProcess();

    ClassDefOverride(TRestDetectorHitsToTrackProcess, 2);  // Template for a REST "event process" class
                                                           // inherited from TRestEventProcess
};
#endif
