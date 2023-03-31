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
/// TRestDetectorSignalToRawSignalProcess transforms a
/// TRestDetectorSignalEvent into a TRestRawSignalEvent. The TRestDetectorSignalEvent
/// contains signal data built with arbitrary times and their corresponding
/// data values (time, data). The data inside a TRestRawSignal losses
/// precision on the time definition, and it is just a data array with a
/// fixed number of data points. Therefore, the time between two consecutive
/// data points in a raw signal event must be kept constant.
///
/// This process produces the sampling of a TRestDetectorSignalEvent into a
/// TRestRawSignalEvent. TRestDetectorSignal contains Float_t data values, while
/// TResRawSignal contains Short_t values. Thats why there might be some
/// information loss when transferring the signal data to the raw-signal data.
/// To minimize the impact, the maximum data value of the output signals should
/// be high enough, and adjusted to the maximum value of a Short_t, being
/// this value 32768. The *gain* parameter may serve to re-adjust the
/// amplitude of the output data array.
///
/// \warning If the value assigned to a data point in the output rawsignal
/// event exceeds 32768 it will cause an overflow, and the event data will
/// be corrupted. If the verboseLevel of the process is warning, an output
/// message will prevent the user. The event status will be invalid.
///
/// The input signal contains arbitrary times expressed in microseconds.
/// In order to produce the binning, a time window must be defined. The
/// parameter *triggerMode* will allow to define how we choose the time
/// start (corresponding to the bin 0 in the raw signal), and time end
/// (corresponding to the last bin in the raw signal).
///
/// The trigger mode will fix the time the signal starts, while the
/// *sampling* time parameter (in microseconds) and the number of points
/// per signal, *Npoints*, will fix the time end. A *triggerDelay*
/// parameter allows to shift the time measured in number of samples, from
/// the definition obtained using the *triggerMode* parameter.
///
/// The following list describes the different parameters that can be
/// used in this process.
///
/// * **sampling**: It is the sampling time of the resulting raw signal
/// output data. Time units must be specified (ns, us, ms)".
///
/// * **Npoints**: The number of points of the resulting raw signals.
///
/// * **triggerMode**: It defines how the start time is fixed. The
/// different options are:
///
///   - *firstDeposit*: The first time deposit found in the event
///     will correspond to the bin 0.
///   - *integralThreshold*: An integral window with size **Npoints/2**
///     will start to scan the input signal event from the first time
///     deposit. The time at which the value of this integral is above
///     the value provided at the **integralThreshold** parameter will
///     be defined as the center of the acquisition window.
///
/// * **integralThreshold**: It defines the value to be used in the
///     triggerThreshold method. This parameter is not used otherwise.
///
///
/// \htmlonly <style>div.image img[src="trigger.png"]{width:500px;}</style> \endhtmlonly
///
/// The following figure shows the integralThreshold trigger definition for a NLDBD
/// event. fTimeStart and fTimeEnd define the acquisition window, centered on the time
/// when the signal integral is above the threshold defined. fTimeStart has been
/// shifted by a triggerDelay = 60 samples * 200ns
///
/// ![An illustration of the trigger definition](trigger.png)
///
/// * **triggerDelay**: The time start obtained by the trigger mode
/// definition can be shifted using this parameter. The shift is
/// measured in number of bins from the output signal.
///
/// * **gain**: Each data point from the resulting raw signal will be
/// multiplied by this factor before performing the conversion to
/// Short_t. Each value in the raw output signal should be between
/// -32768 and 32768, resulting event data will be corrupted otherwise.
/// The state of the event will be set to false fOk=false.
///
///
///--------------------------------------------------------------------------
///
/// RESTsoft - Software for Rare Event Searches with TPCs
///
/// History of developments:
///
/// 2017-November: First implementation of signal to rawsignal conversion.
///             Javier Galan
///
/// \class      TRestDetectorSignalToRawSignalProcess
/// \author     Javier Galan
///
/// <hr>
///

#include "TRestDetectorSignalToRawSignalProcess.h"

using namespace std;

ClassImp(TRestDetectorSignalToRawSignalProcess);

///////////////////////////////////////////////
/// \brief Default constructor
///
TRestDetectorSignalToRawSignalProcess::TRestDetectorSignalToRawSignalProcess() { Initialize(); }

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
TRestDetectorSignalToRawSignalProcess::TRestDetectorSignalToRawSignalProcess(const char* configFilename) {
    Initialize();
    LoadConfig(configFilename);
}

///////////////////////////////////////////////
/// \brief Default destructor
///
TRestDetectorSignalToRawSignalProcess::~TRestDetectorSignalToRawSignalProcess() {
    delete fOutputRawSignalEvent;
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
/// corresponding TRestGeant4AnalysisProcess section inside the RML.
///
void TRestDetectorSignalToRawSignalProcess::LoadConfig(const string& configFilename, const string& name) {
    LoadConfigFromFile(configFilename, name);
}

///////////////////////////////////////////////
/// \brief Function to initialize input/output event members and define the
/// section name
///
void TRestDetectorSignalToRawSignalProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fInputSignalEvent = nullptr;
    fOutputRawSignalEvent = new TRestRawSignalEvent();
}

///////////////////////////////////////////////
/// \brief Some actions taken before start the event data processing
///
void TRestDetectorSignalToRawSignalProcess::InitProcess() {
    if (fTriggerMode == "fixed") {
        fTimeStart = fTriggerFixedStartTime - fTriggerDelay * fSampling;
        fTimeEnd = fTimeStart + fNPoints * fSampling;
    }

    const set<string> validTriggerModes = {"firstDeposit", "integralThreshold", "fixed"};
    if (validTriggerModes.count(fTriggerMode.Data()) == 0) {
        RESTError << "Trigger mode set to: '" << fTriggerMode
                  << "' which is not a valid trigger mode. Please use one of the following trigger modes: ";
        for (const auto& triggerMode : validTriggerModes) {
            RESTError << triggerMode << " ";
        }
        RESTError << RESTendl;
        exit(1);
    }

    if (IsLinearCalibration()) {
        const auto range = numeric_limits<Short_t>::max() - numeric_limits<Short_t>::min();
        fCalibrationGain = range * (fCalibrationRange.Y() - fCalibrationRange.X()) /
                           (fCalibrationEnergy.Y() - fCalibrationEnergy.X());
        fCalibrationOffset = range * (fCalibrationRange.X() - fCalibrationGain * fCalibrationEnergy.X()) +
                             numeric_limits<Short_t>::min();
    }
}

///////////////////////////////////////////////
/// \brief The main processing event function
///
TRestEvent* TRestDetectorSignalToRawSignalProcess::ProcessEvent(TRestEvent* inputEvent) {
    fInputSignalEvent = (TRestDetectorSignalEvent*)inputEvent;

    if (fInputSignalEvent->GetNumberOfSignals() <= 0) {
        return nullptr;
    }

    if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug) {
        fOutputRawSignalEvent->PrintEvent();
    }

    fOutputRawSignalEvent->SetID(fInputSignalEvent->GetID());
    fOutputRawSignalEvent->SetSubID(fInputSignalEvent->GetSubID());
    fOutputRawSignalEvent->SetTimeStamp(fInputSignalEvent->GetTimeStamp());
    fOutputRawSignalEvent->SetSubEventTag(fInputSignalEvent->GetSubEventTag());

    if (fTriggerMode == "firstDeposit") {
        fTimeStart = fInputSignalEvent->GetMinTime() - fTriggerDelay * fSampling;
        fTimeEnd = fInputSignalEvent->GetMinTime() + (fNPoints - fTriggerDelay) * fSampling;
    } else if (fTriggerMode == "integralThreshold") {
        bool thresholdReached = false;
        for (Double_t t = fInputSignalEvent->GetMinTime() - fNPoints * fSampling;
             t <= fInputSignalEvent->GetMaxTime() + fNPoints * fSampling; t = t + 0.5) {
            Double_t energy = fInputSignalEvent->GetIntegralWithTime(t, t + (fSampling * fNPoints) / 2.);

            if (energy > fIntegralThreshold) {
                fTimeStart = t - fTriggerDelay * fSampling;
                fTimeEnd = t + (fNPoints - fTriggerDelay) * fSampling;
                thresholdReached = true;
            }
        }
        if (!thresholdReached) {
            RESTWarning << "Integral threshold for trigger not reached" << RESTendl;
            fTimeStart = 0;
            fTimeEnd = fNPoints * fSampling;
        }
    }

    RESTDebug << "fTimeStart : " << fTimeStart << " us " << RESTendl;
    RESTDebug << "fTimeEnd : " << fTimeEnd << " us " << RESTendl;

    for (int n = 0; n < fInputSignalEvent->GetNumberOfSignals(); n++) {
        vector<Double_t> data(fNPoints, fCalibrationOffset);
        TRestDetectorSignal* signal = fInputSignalEvent->GetSignal(n);
        Int_t signalID = signal->GetSignalID();

        for (int m = 0; m < signal->GetNumberOfPoints(); m++) {
            Double_t t = signal->GetTime(m);
            Double_t d = signal->GetData(m);

            if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug && n < 3 && m < 5) {
                cout << "Signal : " << n << " Sample : " << m << " T : " << t << " Data : " << d << endl;
            }

            if (t > fTimeStart && t < fTimeEnd) {
                // convert physical time (in us) to timeBin
                Int_t timeBin = (Int_t)round((t - fTimeStart) / fSampling);

                if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Warning) {
                    if (timeBin < 0 || timeBin > fNPoints) {
                        cout << "Time bin out of range!!! bin value : " << timeBin << endl;
                        timeBin = 0;
                    }
                }

                RESTDebug << "Adding data : " << signal->GetData(m) << " to Time Bin : " << timeBin
                          << RESTendl;
                data[timeBin] += fCalibrationGain * signal->GetData(m);
            }
        }

        if (IsShapingEnabled()) {
            const auto shapingFunction = [](Double_t t) -> Double_t {
                if (t <= 0) {
                    return 0;
                }
                // function is normalized such that its absolute maximum is 1.0
                // max is at x = 1.1664004483744728
                return TMath::Exp(-3.0 * t) * TMath::Power(t, 3.0) * TMath::Sin(t) * 22.68112123672292;
            };

            vector<Double_t> dataAfterShaping(fNPoints, fCalibrationOffset);
            for (int i = 0; i < fNPoints; i++) {
                const Double_t value = data[i] - fCalibrationOffset;
                if (value <= 0) {
                    // Only positive values are possible, 0 means no signal in this bin
                    continue;
                }
                for (int j = 0; j < fNPoints; j++) {
                    dataAfterShaping[j] += value * shapingFunction(((j - i) * fSampling) / fShapingTime);
                }
            }
            data = dataAfterShaping;
        }

        TRestRawSignal rawSignal;
        rawSignal.SetSignalID(signalID);
        for (int x = 0; x < fNPoints; x++) {
            double value = round(data[x]);
            if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Warning) {
                if (value < numeric_limits<Short_t>::min() || value > numeric_limits<Short_t>::max()) {
                    RESTWarning << "value (" << value << ") is outside short range ("
                                << numeric_limits<Short_t>::min() << ", " << numeric_limits<Short_t>::max()
                                << ")" << RESTendl;
                }
            }

            if (value < numeric_limits<Short_t>::min()) {
                value = numeric_limits<Short_t>::min();
                fOutputRawSignalEvent->SetOK(false);
            } else if (value > numeric_limits<Short_t>::max()) {
                value = numeric_limits<Short_t>::max();
                fOutputRawSignalEvent->SetOK(false);
            }
            rawSignal.AddPoint((Short_t)value);
        }

        if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug) {
            rawSignal.Print();
        }
        RESTDebug << "Adding signal to raw signal event" << RESTendl;

        fOutputRawSignalEvent->AddSignal(rawSignal);
    }

    RESTDebug << "TRestDetectorSignalToRawSignalProcess. Returning event with N signals "
              << fOutputRawSignalEvent->GetNumberOfSignals() << RESTendl;

    return fOutputRawSignalEvent;
}
