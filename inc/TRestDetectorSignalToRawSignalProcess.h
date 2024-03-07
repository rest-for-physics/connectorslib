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

#include <TRestDetectorReadout.h>
#include <TRestDetectorSignalEvent.h>
#include <TRestEventProcess.h>
#include <TRestRawSignalEvent.h>

//! A process to convert a TRestDetectorSignalEvent into a TRestRawSignalEvent
class TRestDetectorSignalToRawSignalProcess : public TRestEventProcess {
   private:
    /// A pointer to the specific TRestDetectorSignalEvent input
    TRestDetectorSignalEvent* fInputSignalEvent;  //!

    /// A pointer to the specific TRestRawSignalEvent input
    TRestRawSignalEvent* fOutputRawSignalEvent;  //!

    TRestDetectorReadout* fReadout = nullptr;  //!

    void Initialize() override;

    void InitFromConfigFile() override;

   protected:
    /// The sampling time from the binned raw output signal
    Double_t fSampling = 1.0;  // ns

    /// The number of points of the resulting output signal
    Int_t fNPoints = 512;

    /// It is used to define the way the time start will be fixed
    std::string fTriggerMode = "firstDeposit";

    /// The number of time bins the time start is delayed in the resulting output signal.
    Int_t fTriggerDelay = 100;

    /// The starting time for the "fixed" trigger mode (can be offset by the trigger delay)
    Int_t fTriggerFixedStartTime = 0;

    /// The name of the observable used to define the trigger mode (i.e. g4Ana_sensitiveVolumeFirstHitTime)
    std::string fTriggerModeObservableName;

    /// fCalibrationGain and fCalibrationOffset define the linear calibration.
    /// output = input * fCalibrationGain + calibrationOffset
    Double_t fCalibrationGain = 100.0;
    Double_t fCalibrationOffset = 0.0;  // adc units

    /// This parameter is used by integralWindow trigger mode to define the acquisition window.
    Double_t fIntegralThreshold = 1229.0;
    Double_t fIntegralThresholdTPCkeV = 0.1;

    /// two distinct energy values used for calibration
    TVector2 fCalibrationEnergy = TVector2(0.0, 0.0);
    /// position in the range corresponding to the energy in 'fCalibrationEnergy'. Values between 0 and 1
    TVector2 fCalibrationRange = TVector2(0.0, 0.0);
    /// Usage: fCalibrationEnergy = (0, 100 MeV) and fCalibrationRange = (0.1, 0.9)
    /// will perform a linear calibration with 0 equal to 0.1 of the range (0.1 * (max - min) + min) and 100
    /// MeV equal to 0.9 of the range. The range is the one corresponding to a Short_t for rawsignal.

    /// If defined ( > 0 ) we will compute the sin shaping of the signal, this is done in this process to
    /// avoid artifacts in the signal (e.g. signals not getting cut when they should)
    Double_t fShapingTime = 0.0;  // us

   public:
    inline Double_t GetSampling() const { return fSampling; }

    inline Int_t GetNPoints() const { return fNPoints; }

    inline std::string GetTriggerMode() const { return fTriggerMode; }

    inline Int_t GetTriggerDelay() const { return fTriggerDelay; }

    inline Double_t GetGain() const { return fCalibrationGain; }

    inline Double_t GetIntegralThreshold() const { return fIntegralThreshold; }

    inline bool IsLinearCalibration() const {
        // Will return true if two points have been given for calibration
        return (fCalibrationEnergy.Mod() != 0 && fCalibrationRange.Mod() != 0);
    }

    RESTValue GetInputEvent() const override { return fInputSignalEvent; }

    RESTValue GetOutputEvent() const override { return fOutputRawSignalEvent; }

    Double_t GetEnergyFromADC(Double_t adc, const std::string& type = "") const;

    Double_t GetADCFromEnergy(Double_t energy, const std::string& type = "") const;

    Double_t GetTimeFromBin(Double_t bin, const std::string& type = "") const;

    Double_t GetBinFromTime(Double_t time, const std::string& type = "") const;

    struct Parameters {
        Double_t sampling = 1.0;
        Double_t shapingTime = 0.0;
        Double_t calibrationGain = 100;
        Double_t calibrationOffset = 0;
        TVector2 calibrationEnergy = {0, 0};
        TVector2 calibrationRange = {0, 0};
    };

    void InitProcess() override;

    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;

    void LoadConfig(const std::string& configFilename, const std::string& name = "");

    /// It prints out the process parameters stored in the metadata structure
    void PrintMetadata() override;

    /// Returns a new instance of this class
    TRestEventProcess* Maker() { return new TRestDetectorSignalToRawSignalProcess; }

    /// Returns the name of this process
    const char* GetProcessName() const override { return "signalToRawSignal"; }

    // Constructor
    TRestDetectorSignalToRawSignalProcess();

    TRestDetectorSignalToRawSignalProcess(const char* configFilename);

    // Destructor
    ~TRestDetectorSignalToRawSignalProcess();

   private:
    std::map<std::string, Parameters> fParametersMap;
    std::set<std::string> fReadoutTypes;

    ClassDefOverride(TRestDetectorSignalToRawSignalProcess, 7);
};

#endif
