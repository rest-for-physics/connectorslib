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
/// The TRestDetectorHitsToTrackProcess transforms a TRestDetectorHitsEvent
/// into a TRestTrackEvent. It creates tracks or clusters (groups of hits)
/// that have a relation of proximity. If a group of hits distance to another
/// group of hits is larger than the `clusterDistance` parameter, then the
/// groups, or tracks, will be considered independent inside the
/// TRestTrackEvent.
///
/// This process evaluates all hit interdistances using the `clusterDistance`
/// parameter. Therefore, for many hits events the process might slow down.
/// An approximate method for hit to track clustering is implemented at
/// the TRestDetectorHitsToTrackFastProcess.
///
/// The following list describes the different parameters that can be
/// used in this process.
///
/// * **clusterDistance**: It is the distance at which two hits are
/// considered to belong to the same group of hits.
///
/// The following lines of code show how the process metadata should be
/// defined.
///
/// Basic definition inside a processing chain construction at
/// TRestProcessRunner
///
/// \code
///
/// <addProcess type="TRestDetectorHitsToTrackProcess name="hitsToTrack"
///				clusterDistance="2.5mm" />
///
/// \endcode
///
/// <hr>
///
/// \warning **⚠ WARNING: REST is under continous development.** This documentation
/// is offered to you by the REST community. Your HELP is needed to keep this code
/// up to date. Your feedback will be worth to support this software, please report
/// any problems/suggestions you may find while using it at [The REST Framework
/// forum](http://ezpc10.unizar.es). You are welcome to contribute fixing typos, updating
/// information or adding/proposing new contributions. See also our [Contribution
/// Guide](https://github.com/rest-for-physics/framework/blob/master/CONTRIBUTING.md)
///
///--------------------------------------------------------------------------
///
/// RESTsoft - Software for Rare Event Searches with TPCs
///
/// History of developments:
///
/// 2015-December: First implementation of hits to track process
///                Javier Gracia
///
/// 2016-January: Adapted to get tracks in bi-dimensional hits
/// 2022-January: Documented and added official headers
///             Javier Galan
///
/// \class      TRestDetectorHitsToTrackProcess
/// \author     Javier Gracia
/// \author     Javier Galan
///
/// <hr>
///

#include "TRestDetectorHitsToTrackProcess.h"
using namespace std;

ClassImp(TRestDetectorHitsToTrackProcess);

///////////////////////////////////////////////
/// \brief Default constructor
///
TRestDetectorHitsToTrackProcess::TRestDetectorHitsToTrackProcess() { Initialize(); }

///////////////////////////////////////////////
/// \brief Default destructor
///
TRestDetectorHitsToTrackProcess::~TRestDetectorHitsToTrackProcess() {
    delete fTrackEvent;
    // TRestDetectorHitsToTrackProcess destructor
}

///////////////////////////////////////////////
/// \brief Function to initialize input/output event members and define the
/// section name
///
void TRestDetectorHitsToTrackProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fClusterDistance = 1.;

    fHitsEvent = nullptr;
    fTrackEvent = new TRestTrackEvent();
}

///////////////////////////////////////////////
/// \brief The main processing event function
///
TRestEvent* TRestDetectorHitsToTrackProcess::ProcessEvent(TRestEvent* inputEvent) {
    /* Time measurement
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    */

    fHitsEvent = (TRestDetectorHitsEvent*)inputEvent;
    fTrackEvent->SetEventInfo(fHitsEvent);

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug)
        cout << "TResDetectorHitsToTrackProcess : nHits " << fHitsEvent->GetNumberOfHits() << endl;

    TRestHits* xzHits = fHitsEvent->GetXZHits();

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug)
        cout << "TRestDetectorHitsToTrackProcess : Number of xzHits : " << xzHits->GetNumberOfHits() << endl;
    Int_t xTracks = FindTracks(xzHits);

    fTrackEvent->SetNumberOfXTracks(xTracks);

    TRestHits* yzHits = fHitsEvent->GetYZHits();
    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug)
        cout << "TRestDetectorHitsToTrackProcess : Number of yzHits : " << yzHits->GetNumberOfHits() << endl;
    Int_t yTracks = FindTracks(yzHits);

    fTrackEvent->SetNumberOfYTracks(yTracks);

    TRestHits* xyzHits = fHitsEvent->GetXYZHits();
    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug)
        cout << "TRestDetectorHitsToTrackProcess : Number of xyzHits : " << xyzHits->GetNumberOfHits()
             << endl;

    FindTracks(xyzHits);

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug) {
        cout << "TRestDetectorHitsToTrackProcess. X tracks : " << xTracks << "  Y tracks : " << yTracks
             << endl;
        cout << "TRestDetectorHitsToTrackProcess. Total number of tracks : "
             << fTrackEvent->GetNumberOfTracks() << endl;
    }

    if (fTrackEvent->GetNumberOfTracks() == 0) return nullptr;

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug)
        fTrackEvent->PrintOnlyTracks();

    fTrackEvent->SetLevels();

    return fTrackEvent;
}

///////////////////////////////////////////////
/// \brief The main algorithm. It idetifies the hits that belong to
/// each track and adds them already to the output TRestTrackEvent.
///
/// \return It returns the number of tracks found
///
Int_t TRestDetectorHitsToTrackProcess::FindTracks(TRestHits* hits) {
    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Extreme) hits->PrintHits();
    Int_t nTracksFound = 0;
    vector<Int_t> Q;  // list of points (hits) that need to be checked
    vector<Int_t> P;  // list of neighbours within a radious fClusterDistance

    bool isProcessed = false;
    Int_t qsize = 0;
    TRestTrack* track = new TRestTrack();

    TRestVolumeHits volHit;

    Float_t fClusterDistance2 = (Float_t)(fClusterDistance * fClusterDistance);

    // for every event in the point cloud
    while (hits->GetNumberOfHits() > 0) {
        Q.push_back(0);

        // for every point in Q
        for (unsigned int q = 0; q < Q.size(); q++) {
            // we look for the neighbours
            for (unsigned int j = 0; j < hits->GetNumberOfHits(); j++) {
                if ((int)j != Q[q]) {
                    if (hits->GetDistance2(Q[q], j) < fClusterDistance2) P.push_back(j);
                }
            }
            qsize = Q.size();

            // For all the neighbours found P.size()
            // Check if the points have already been processed
            for (unsigned int i = 0; i < P.size(); i++) {
                isProcessed = false;

                for (int j = 0; j < qsize; j++) {
                    // if yes, we do not consider it again
                    if (P[i] == Q[j]) {
                        isProcessed = true;
                        break;
                    }
                }

                // If not, we add the point P[i] to the list of Q
                if (isProcessed == false) {
                    Q.push_back(P[i]);
                }
            }

            P.clear();
        }

        // We order the Q vector
        std::sort(Q.begin(), Q.end());
        // Then we swap to decresing order
        std::reverse(Q.begin(), Q.end());

        // When the list of all points in Q has been processed, we add the clusters
        // to the TrackEvent and reset Q
        for (unsigned int nhit = 0; nhit < Q.size(); nhit++) {
            const Double_t x = hits->GetX(Q[nhit]);
            const Double_t y = hits->GetY(Q[nhit]);
            const Double_t z = hits->GetZ(Q[nhit]);
            const Double_t en = hits->GetEnergy(Q[nhit]);

            TVector3 pos(x, y, z);
            TVector3 sigma(0., 0., 0.);

            volHit.AddHit(pos, en, 0, hits->GetType(Q[nhit]), sigma);

            hits->RemoveHit(Q[nhit]);
        }

        track->SetParentID(0);
        track->SetTrackID(fTrackEvent->GetNumberOfTracks() + 1);
        track->SetVolumeHits(volHit);
        volHit.RemoveHits();

        if (Q.size() > 1 || !fIgnoreOneHitTracks) {
            RESTDebug << "Adding track : id=" << track->GetTrackID() << " parent : " << track->GetParentID()
                    << RESTendl;
            fTrackEvent->AddTrack(track);
            nTracksFound++;
        }

        Q.clear();
    }

    delete track;

    return nTracksFound;
}

void TRestDetectorHitsToTrackProcess::InitFromConfigFile() {
    fClusterDistance = StringToDouble(GetParameter("clusterDistance", fClusterDistance));
    fIgnoreOneHitTracks = StringToBool(GetParameter("ignoreOneHitTracks", fIgnoreOneHitTracks));
}