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
///
/// RESTsoft - Software for Rare Event Searches with TPCs
///
/// History of developments:
///
///
/// \class TRestDetectorRawSignalToHitsProcess
///
/// <hr>
///
#include "TRestDetectorRawSignalToHitsProcess.h"

using namespace std;

ClassImp(TRestDetectorRawSignalToHitsProcess);

///////////////////////////////////////////////
/// \brief Default constructor
///
TRestDetectorRawSignalToHitsProcess::TRestDetectorRawSignalToHitsProcess() {
    Initialize();
}

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
TRestDetectorRawSignalToHitsProcess::TRestDetectorRawSignalToHitsProcess(char* cfgFileName) {
    Initialize();

    if (LoadConfigFromFile(cfgFileName) == -1) LoadDefaultConfig();
    PrintMetadata();
}

///////////////////////////////////////////////
/// \brief Function to load the default config in absence of RML input
///
void TRestDetectorRawSignalToHitsProcess::LoadDefaultConfig() { SetTitle("Default config"); }

///////////////////////////////////////////////
/// \brief Default destructor
///
TRestDetectorRawSignalToHitsProcess::~TRestDetectorRawSignalToHitsProcess() {
    delete fHitsEvent;
}

///////////////////////////////////////////////
/// \brief Function to initialize input/output event members and define the
/// section name
///
void TRestDetectorRawSignalToHitsProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fHitsEvent = new TRestDetectorHitsEvent();
    fRawSignalEvent = 0;

    fGas = NULL;
    fReadout = NULL;
}

///////////////////////////////////////////////
/// \brief Process initialization. Observable names are interpreted and auxiliar
/// observable members, related to VolumeEdep, MeanPos, TracksCounter, TrackEDep
/// observables defined in TRestGeant4AnalysisProcess are filled at this stage.
///
void TRestDetectorRawSignalToHitsProcess::InitProcess() {

    fGas = GetMetadata<TRestDetectorGas>();
    if (fGas != NULL) {

#ifndef USE_Garfield
        ferr << "A TRestDetectorGas definition was found but REST was not linked to Garfield libraries."
             << endl;
        ferr << "Please, remove the TRestDetectorGas definition, and add gas parameters inside the process "
                "TRestDetectorRawSignalToHitsProcess"
             << endl;
        if (!fGas->GetError()) fGas->SetError("REST was not compiled with Garfield.");
        if (!this->GetError()) this->SetError("Attempt to use TRestDetectorGas without Garfield");
#endif

        if (fGasPressure <= 0) fGasPressure = fGas->GetPressure();
        if (fElectricField <= 0) fElectricField = fGas->GetElectricField();

        fGas->SetPressure(fGasPressure);
        fGas->SetElectricField(fElectricField);

        if (fDriftVelocity <= 0) fDriftVelocity = fGas->GetDriftVelocity();
    } else {
        if (fDriftVelocity < 0) {
            if (!this->GetError()) this->SetError("Drift velocity is negative.");
        }
    }

    fReadout = GetMetadata<TRestDetectorReadout>();

    if (fReadout == nullptr) {
        if (!this->GetError()) this->SetError("The readout was not properly initialized.");
    }
}

///////////////////////////////////////////////
/// \brief The main processing event function
///

TRestEvent* TRestDetectorRawSignalToHitsProcess::ProcessEvent(TRestEvent* evInput) {
    fRawSignalEvent = (TRestRawSignalEvent*)evInput;

    if (!fReadout) return nullptr;

    fHitsEvent->SetID(fRawSignalEvent->GetID());
    fHitsEvent->SetSubID(fRawSignalEvent->GetSubID());
    fHitsEvent->SetTimeStamp(fRawSignalEvent->GetTimeStamp());
    fHitsEvent->SetSubEventTag(fRawSignalEvent->GetSubEventTag());

    debug << "TRestDetectorRawSignalToHitsProcess. Event id : " << fHitsEvent->GetID() << endl;
    if (GetVerboseLevel() == REST_Extreme) fRawSignalEvent->PrintEvent();

    Int_t numberOfSignals = fRawSignalEvent->GetNumberOfSignals();

    Int_t planeID, readoutChannel = -1, readoutModule;
    for (int i = 0; i < numberOfSignals; i++) {
        TRestRawSignal* sgnl = fRawSignalEvent->GetSignal(i);
        Int_t signalID = sgnl->GetSignalID();

        if (GetVerboseLevel() >= REST_Debug)
            cout << "Searching readout coordinates for signal ID : " << signalID << endl;

        fReadout->GetPlaneModuleChannel(signalID, planeID, readoutModule, readoutChannel);

        if (readoutChannel == -1) {
            cout << "Warning : Readout channel not found for daq ID : " << signalID << endl;
            continue;
        }
        /////////////////////////////////////////////////////////////////////////

        TRestDetectorReadoutPlane* plane = fReadout->GetReadoutPlaneWithID(planeID);

        // For the moment this will only be valid for a TPC with its axis (field
        // direction) being in z
        Double_t fieldZDirection = plane->GetPlaneVector().Z();
        Double_t zPosition = plane->GetPosition().Z();

        Double_t x = plane->GetX(readoutModule, readoutChannel);
        Double_t y = plane->GetY(readoutModule, readoutChannel);

        REST_HitType type = XYZ;
        TRestDetectorReadoutModule* mod = plane->GetModuleByID(readoutModule);
        if (TMath::IsNaN(x)) {
            x = mod->GetPhysicalCoordinates(TVector2(mod->GetModuleSizeX() / 2, mod->GetModuleSizeY() / 2)).X();
            type = YZ;
            debug<<"SignalID "<<signalID<<" ReadoutChannel "<< readoutChannel<<" y: "<<y<<endl;
        } else if (TMath::IsNaN(y)) {
            y = mod->GetPhysicalCoordinates(TVector2(mod->GetModuleSizeX() / 2, mod->GetModuleSizeY() / 2)).Y();
            debug<<"SignalID "<<signalID<<" ReadoutChannel "<< readoutChannel<<" x: "<<x<<endl;
            type = XZ;
        }

        if (fMethod == "onlyMax") {
            Double_t time = sgnl->GetMaxPeakBin()*fSampling + fTriggerStarts;
            Double_t distanceToPlane = time * fDriftVelocity;

            debug << "Distance to plane : " << distanceToPlane << endl;

            Double_t z = zPosition + fieldZDirection * distanceToPlane;

            Double_t energy = sgnl->GetMaxPeakValue();

            debug << "Adding hit. Time : " << time << " x : " << x << " y : " << y << " z : " << z << " Energy : " << energy << endl;

            fHitsEvent->AddHit(x, y, z, energy, 0, type);
        } else if (fMethod == "tripleMax") {
            Int_t bin = sgnl->GetMaxPeakBin();
            int binprev = (bin - 1) < 0 ? bin : bin - 1;
            int binnext = (bin + 1) > sgnl->GetNumberOfPoints() - 1 ? bin : bin + 1;

            Double_t time = bin*fSampling + fTriggerStarts;
            Double_t energy = sgnl->GetData(bin);

            Double_t distanceToPlane = time * fDriftVelocity;
            Double_t z = zPosition + fieldZDirection * distanceToPlane;

            fHitsEvent->AddHit(x, y, z, energy, 0, type);

            time = binprev*fSampling  + fTriggerStarts;
            energy = sgnl->GetData(binprev);

            distanceToPlane = time * fDriftVelocity;
            z = zPosition + fieldZDirection * distanceToPlane;

            fHitsEvent->AddHit(x, y, z, energy, 0, type);

            time = binnext*fSampling + fTriggerStarts;
            energy = sgnl->GetData(binnext);

            distanceToPlane = time * fDriftVelocity;
            z = zPosition + fieldZDirection * distanceToPlane;

            fHitsEvent->AddHit(x, y, z, energy, 0, type);

            debug << "Distance to plane : " << distanceToPlane << "\nAdding hit. Time : " << time << " x : " << x << " y : " << y << " z : " << z << " Energy : " << energy << endl;
        } else if (fMethod == "qCenter") {
            Double_t energy_signal = 0;
            Double_t distanceToPlane = 0;

            for (int j = 0; j < sgnl->GetNumberOfPoints(); j++) {
                Double_t energy_point = sgnl->GetData(j);
                energy_signal += energy_point;
                distanceToPlane += (j * fSampling  + fTriggerStarts) * fDriftVelocity * energy_point;
            }
            Double_t energy = energy_signal / sgnl->GetNumberOfPoints();

            Double_t z = zPosition + fieldZDirection * (distanceToPlane / energy_signal);
            fHitsEvent->AddHit(x, y, z, energy, 0, type);
        } else if (fMethod == "all" ) {
            for (int j = 0; j < sgnl->GetNumberOfPoints(); j++) {
                Double_t energy = sgnl->GetData(j);

                Double_t distanceToPlane = (j * fSampling  + fTriggerStarts) * fDriftVelocity;

                debug << "Time : " << j * fSampling + fTriggerStarts << " Drift velocity : " << fDriftVelocity << "\nDistance to plane : " << distanceToPlane << endl;

                Double_t z = zPosition + fieldZDirection * distanceToPlane;

                debug << "Adding hit. Time : " << j * fSampling  + fTriggerStarts << " x : " << x << " y : " << y << " z : " << z << endl;

                fHitsEvent->AddHit(x, y, z, energy, 0, type);
            }
        } else if (fMethod == "intwindow" ) {
          Int_t nPoints = sgnl->GetNumberOfPoints();
          debug<<"Number of points "<<nPoints<<endl;
          sgnl->CalculateBaseLine(fBaseLineRange.X(),fBaseLineRange.Y());
          double baseline = sgnl->GetBaseLine();
          double baselineSigma = sgnl->GetBaseLineSigma();
          debug<<"Baseline "<<baseline<<" BaselineSigma "<<baselineSigma<<endl;

            for(int j= 0; j<nPoints-fIntWindow;j+=fIntWindow){
              double energy =0;
                for(int p=0;p<fIntWindow;p++){
                  //debug<<j+p<<" "<<sgnl->GetRawData(j+p)<<endl;
                  energy += (double)sgnl->GetData(j+p);
                }

                if(fIntWindow>0) energy /= (double)fIntWindow;
                //debug<<"TimeBin "<<j + (fIntWindow-1)/2. <<" Charge "<<energy<<" "<<" Baseline "<<baseline<<endl;
                if(energy <  fThreshold ) continue;
              debug<<"TimeBin "<<j<<"-"<<j+(fIntWindow -1)<<" Charge: "<<energy <<" Thr: " <<(fThreshold)<<endl;
              Double_t time = (j + (fIntWindow-1)/2. ) * fSampling  + fTriggerStarts;
              Double_t distanceToPlane = time * fDriftVelocity;
              Double_t z = zPosition + fieldZDirection * distanceToPlane;

              debug << "Time : " << time << " Drift velocity : " << fDriftVelocity << "\nDistance to plane : " << distanceToPlane << endl;
              debug << "Adding hit. Time : " << time << " x : " << x << " y : " << y << " z : " << z <<" type "<<type<<endl;

                fHitsEvent->AddHit(x, y, z, energy, 0, type);
            }
        } else
	{
		string errMsg = "The method " + (string) fMethod + " is not implemented!";
		SetError( errMsg );
	}
    }

    debug << "TRestDetectorRawSignalToHitsProcess. Hits added : " << fHitsEvent->GetNumberOfHits() << endl;
    debug << "TRestDetectorRawSignalToHitsProcess. Hits total energy : " << fHitsEvent->GetEnergy() << endl;

    if (this->GetVerboseLevel() == REST_Debug) {
        fHitsEvent->PrintEvent(30);
    } else if (this->GetVerboseLevel() == REST_Extreme) {
        fHitsEvent->PrintEvent(-1);
    }

    if (fHitsEvent->GetNumberOfHits() <= 0) {
        string errMsg = "Last event id: " + IntegerToString(fHitsEvent->GetID()) +
                        ". Failed to find readout positions in channel to hit conversion.";
        SetWarning(errMsg);
        return nullptr;
    }

    return fHitsEvent;
}

void TRestDetectorRawSignalToHitsProcess::InitFromConfigFile() {
    fSampling = GetDblParameterWithUnits("sampling");
    fTriggerStarts = GetDblParameterWithUnits("triggerStarts");
    fBaseLineRange = StringTo2DVector(GetParameter("baseLineRange", "(10,150)"));
    fElectricField = GetDblParameterWithUnits("electricField", 0.);
    fGasPressure = GetDblParameterWithUnits("gasPressure", -1.);
    fDriftVelocity = GetDblParameterWithUnits("driftVelocity", 0.);
    fThreshold = StringToDouble(GetParameter("threshold", "100"));
    fIntWindow = StringToDouble(GetParameter("intwindow", "3"));
}

