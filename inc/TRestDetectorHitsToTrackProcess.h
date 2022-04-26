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

    void Initialize();
    Int_t FindTracks(TRestHits* hits);

   protected:
    /// The hits distance used to define a cluster of hits
    Double_t fClusterDistance = 2.5;

   public:
    inline any GetInputEvent() const { return fHitsEvent; }
    inline any GetOutputEvent() const { return fTrackEvent; }

    TRestEvent* ProcessEvent(TRestEvent* eventInput);

    /// It prints out the process parameters stored in the metadata structure
    void PrintMetadata() {
        BeginPrintProcess();

        metadata << " cluster-distance : " << fClusterDistance << " mm " << endl;

        EndPrintProcess();
    }

    /// Returns the name of this process
    inline TString GetProcessName() const { return (TString) "hitsToTrack"; }

    TRestDetectorHitsToTrackProcess();
    ~TRestDetectorHitsToTrackProcess();

    ClassDef(TRestDetectorHitsToTrackProcess, 1);  // Template for a REST "event process" class inherited from
                                                   // TRestEventProcess
};
#endif
