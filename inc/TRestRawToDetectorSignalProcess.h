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

#ifndef RestCore_TRestRawToDetectorSignalProcess
#define RestCore_TRestRawToDetectorSignalProcess

#include <TRestDetectorSignalEvent.h>
#include <TRestRawSignalEvent.h>

#include "TRestEventProcess.h"

//! A process to convert a TRestRawSignalEvent into a TRestDetectorSignalEvent
class TRestRawToDetectorSignalProcess : public TRestEventProcess {
   private:
    /// A pointer to the specific TRestRawSignalEvent input
    TRestRawSignalEvent* fInputSignalEvent;  //!

    /// A pointer to the specific TRestDetectorSignalEvent input
    TRestDetectorSignalEvent* fOutputSignalEvent;  //!

    void Initialize() override;

   protected:
    /// The sampling time used to transform the binned data to time information
    Double_t fSampling = 0.1;

    /// The corresponding time of the first bin inside the raw signal
    Double_t fTriggerStarts = 0;

    /// A factor the data values will be multiplied by at the output signal.
    Double_t fGain = 1;

    /// A factor the data values will be multiplied by at the output signal.
    Double_t fThreshold = 0.1;

    // Perform Zero suppression to the data
    Bool_t fZeroSuppression = false;

    /// The ADC range used for baseline offset definition
    TVector2 fBaseLineRange = TVector2(5, 55);

    /// The ADC range used for integral definition and signal identification
    TVector2 fIntegralRange = TVector2(10, 500);

    /// Number of sigmas over baseline fluctuation to accept a point is over threshold.
    Double_t fPointThreshold = 3;

    /// A threshold parameter to accept or reject a pre-identified signal. See process description.
    Double_t fSignalThreshold = 5;

    /// Number of consecutive points over threshold required to accept a signal.
    Int_t fNPointsOverThreshold = 5;

    /// A parameter to determine if baseline correction has been applied by a previous process
    Bool_t fBaseLineCorrection = false;

   public:
    any GetInputEvent() const override { return fInputSignalEvent; }
    any GetOutputEvent() const override { return fOutputSignalEvent; }

    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;

    void ZeroSuppresion(TRestRawSignal* rawSignal, TRestDetectorSignal& sgnl);

    /// It prints out the process parameters stored in the metadata structure
    void PrintMetadata() override {
        BeginPrintProcess();

        RESTMetadata << "Sampling time : " << fSampling << " us" << RESTendl;
        RESTMetadata << "Trigger starts : " << fTriggerStarts << " us" << RESTendl;
        RESTMetadata << "Gain : " << fGain << RESTendl;

        if (fZeroSuppression) {
            RESTMetadata << "Base line range definition : ( " << fBaseLineRange.X() << " , "
                         << fBaseLineRange.Y() << " ) " << RESTendl;
            RESTMetadata << "Integral range : ( " << fIntegralRange.X() << " , " << fIntegralRange.Y()
                         << " ) " << RESTendl;
            RESTMetadata << "Point Threshold : " << fPointThreshold << " sigmas" << RESTendl;
            RESTMetadata << "Signal threshold : " << fSignalThreshold << " sigmas" << RESTendl;
            RESTMetadata << "Number of points over threshold : " << fNPointsOverThreshold << RESTendl;
        }

        if (fBaseLineCorrection)
            RESTMetadata << "BaseLine correction is enabled for TRestRawSignalAnalysisProcess" << RESTendl;

        EndPrintProcess();
    }

    /// Returns a new instance of this class
    TRestEventProcess* Maker() { return new TRestRawToDetectorSignalProcess; }

    /// Returns the name of this process
    const char* GetProcessName() const override { return "rawSignalToSignal"; }

    // Constructor
    TRestRawToDetectorSignalProcess();

    // Destructor
    ~TRestRawToDetectorSignalProcess();

    ClassDefOverride(TRestRawToDetectorSignalProcess, 2);
};
#endif
