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

#ifndef RestCore_TRestDetectorSignalToRawSignalProcess
#define RestCore_TRestDetectorSignalToRawSignalProcess

#include <TRestDetectorSignalEvent.h>
#include <TRestRawSignalEvent.h>

#include "TRestEventProcess.h"

//! A process to convert a TRestDetectorSignalEvent into a TRestRawSignalEvent
class TRestDetectorSignalToRawSignalProcess : public TRestEventProcess {
   private:
    /// A pointer to the specific TRestDetectorSignalEvent input
    TRestDetectorSignalEvent* fInputSignalEvent;  //!

    /// A pointer to the specific TRestRawSignalEvent input
    TRestRawSignalEvent* fOutputRawSignalEvent;  //!

    void InitFromConfigFile() override;

    void Initialize() override;

   protected:
    /// The sampling time from the binned raw output signal
    Double_t fSampling = 1.0;  // ns

    /// The number of points of the resulting output signal
    Int_t fNPoints = 512;

    /// It is used to define the way the time start will be fixed
    TString fTriggerMode = "firstDeposit";

    /// The number of time bins the time start is delayed in the resulting output signal.
    Int_t fTriggerDelay = 100;  // ns

    /// A factor the data values will be multiplied by at the output signal.
    Double_t fGain = 100.0;

    /// This parameter is used by integralWindow trigger mode to define the acquisition window.
    Double_t fIntegralThreshold = 1229.0;

   public:
    inline Double_t GetSampling() const { return fSampling; }
    inline void SetSampling(Double_t sampling) { fSampling = sampling; }

    inline Int_t GetNPoints() const { return fNPoints; }
    inline void SetNPoints(Int_t nPoints) { fNPoints = nPoints; }

    inline TString GetTriggerMode() const { return fTriggerMode; }
    inline void SetTriggerMode(const TString& triggerMode) { fTriggerMode = triggerMode; }

    inline Int_t GetTriggerDelay() const { return fTriggerDelay; }
    inline void SetTriggerDelay(Int_t triggerDelay) { fTriggerDelay = triggerDelay; }

    inline Double_t GetGain() const { return fGain; }
    inline void SetGain(Double_t gain) { fGain = gain; }

    inline Double_t GetIntegralThreshold() const { return fIntegralThreshold; }
    inline void SetIntegralThreshold(Double_t integralThreshold) { fIntegralThreshold = integralThreshold; }

    any GetInputEvent() const override { return fInputSignalEvent; }
    any GetOutputEvent() const override { return fOutputRawSignalEvent; }

    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;

    void LoadConfig(const std::string& configFilename, const std::string& name = "");

    /// It prints out the process parameters stored in the metadata structure
    void PrintMetadata() override {
        BeginPrintProcess();

        RESTMetadata << "Sampling time : " << fSampling << " us" << RESTendl;
        RESTMetadata << "Points per channel : " << fNPoints << RESTendl;
        RESTMetadata << "Trigger mode : " << fTriggerMode << RESTendl;
        RESTMetadata << "Trigger delay : " << fTriggerDelay << " time units" << RESTendl;
        RESTMetadata << "ADC gain : " << fGain << RESTendl;

        EndPrintProcess();
    }

    /// Returns a new instance of this class
    TRestEventProcess* Maker() { return new TRestDetectorSignalToRawSignalProcess; }

    /// Returns the name of this process
    const char* GetProcessName() const override { return "signalToRawSignal"; }

    // Constructor
    TRestDetectorSignalToRawSignalProcess();
    TRestDetectorSignalToRawSignalProcess(const char* configFilename);

    // Destructor
    ~TRestDetectorSignalToRawSignalProcess();

    ClassDefOverride(TRestDetectorSignalToRawSignalProcess, 2);
};
#endif
