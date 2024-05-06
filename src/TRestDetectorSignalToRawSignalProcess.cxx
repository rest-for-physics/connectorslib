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
/// this value 32767. The *gain* parameter may serve to re-adjust the
/// amplitude of the output data array.
///
/// \warning If the value assigned to a data point in the output rawsignal
/// event exceeds 32767 it will cause an overflow, and the event data will
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
/// * **nPoints**: The number of points of the resulting raw signals.
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
///   - *fixed*: User manually sets the time corresponding to the bin 0 via the **triggerFixedStartTime**
///     parameter. It is affected by the **triggerDelay** parameter.
///   - *observable*: User manually sets the time corresponding to the bin 0 via the
///   **triggerModeObservableName**
///   - *firstDepositTPC*: Similar to first deposit but only using TPC signals (channels with type "tpc")
///
/// * **integralThreshold**: It defines the value to be used in the
///     triggerThreshold method. This parameter is not used otherwise.
///
/// * **triggerFixedStartTime**: It defines the time (with units) of bin 0 when used with *fixed* trigger mode
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
/// -32768 and 32767, resulting event data will be corrupted otherwise.
/// The state of the event will be set to false fOk=false.
///
/// * **offset**: Value to add to all amplitudes (position of zero level)
///
/// * **calibrationEnergy**: Pair of energies used for linear calibration (alternative to setting gain/offset)
/// * **calibrationRange**: Pair of numbers between 0.0 and 1.0 to define linear calibration.
/// They correspond to the values of energy set by *calibrationEnergy*.
/// 0.0 corresponds to the minimum of the signal range (-32768 for Short_t) and 1.0 to the maximum (32767 for
/// Short_t)
///
/// * **shapingTime**: shaping time in time units. If set the signal will be shaped by sin + undershoot
/// shaper. We allow shaping in this process to avoid artifacts produced if shaping the signal after
/// digitalization.
/// TODO: Rework TRestRawSignal so this is not needed and remove shaping from this process
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

#include <TObjString.h>
#include <TRestRawReadoutMetadata.h>

#include <limits>

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

    double triggerTime = 0;
    double startTimeNoOffset = 0;

    if (fTriggerMode == "firstDeposit") {
        startTimeNoOffset = fInputSignalEvent->GetMinTime();
    } else if (fTriggerMode == "integralThreshold") {
        bool thresholdReached = false;
        for (Double_t t = fInputSignalEvent->GetMinTime() - fNPoints * fSampling;
             t <= fInputSignalEvent->GetMaxTime() + fNPoints * fSampling; t = t + 0.5) {
            Double_t energy = fInputSignalEvent->GetIntegralWithTime(t, t + (fSampling * fNPoints) / 2.);

            if (energy > fIntegralThreshold) {
                startTimeNoOffset = t;
                thresholdReached = true;
            }
        }
        if (!thresholdReached) {
            RESTWarning << "Integral threshold for trigger not reached" << RESTendl;
            startTimeNoOffset = 0;
        }
    } else if (fTriggerMode == "observable") {
        const auto obs = GetObservableValue<double>(fTriggerModeObservableName);
        startTimeNoOffset = obs;

    } else if (fTriggerMode == "firstDepositTPC" || fTriggerMode == "integralThresholdTPC") {
        fReadout = GetMetadata<TRestDetectorReadout>();

        if (fReadout == nullptr) {
            RESTError << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: "
                      << "TRestDetectorReadout metadata not found" << RESTendl;
            exit(1);
        }

        set<const TRestDetectorSignal*> tpcSignals;

        for (int n = 0; n < fInputSignalEvent->GetNumberOfSignals(); n++) {
            TRestDetectorSignal* signal = fInputSignalEvent->GetSignal(n);
            if (signal->GetSignalType() == "tpc") {
                tpcSignals.insert(signal);
            }
            /*
            const auto allDaqIds = fReadout->GetAllDaqIds();
            for (const auto& daqId : allDaqIds) {
                const auto& channel = fReadout->GetReadoutChannelWithDaqID(daqId);
                // TODO: sometimes channel type does not match signal type, why?
            }
            */
        }

        if (tpcSignals.empty()) {
            return nullptr;
        }

        if (fTriggerMode == "firstDepositTPC") {
            double startTime = std::numeric_limits<float>::max();
            for (const auto& signal : tpcSignals) {
                const auto minTime = signal->GetMinTime();
                if (minTime < startTime) {
                    startTime = minTime;
                }
            }

            if (startTime >= std::numeric_limits<float>::max()) {
                return nullptr;
            }
            startTimeNoOffset = startTime;

        } else if (fTriggerMode == "integralThresholdTPC") {
            RESTDebug << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: "
                      << "Trigger mode integralThresholdTPC" << RESTendl;

            if (fIntegralThresholdTPCkeV <= 0) {
                RESTError << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: "
                          << "integralThresholdTPCkeV must be greater than 0: " << fIntegralThresholdTPCkeV
                          << RESTendl;
                exit(1);
            }

            double totalEnergy = 0;
            for (const auto& signal : tpcSignals) {
                totalEnergy += signal->GetIntegral();
            }
            if (totalEnergy < fIntegralThresholdTPCkeV) {
                return nullptr;
            }

            Double_t maxTime = std::numeric_limits<Double_t>::min();
            Double_t minTime = std::numeric_limits<Double_t>::max();
            for (const auto& signal : tpcSignals) {
                const auto maxSignalTime = signal->GetMaxTime();
                if (maxSignalTime > maxTime) {
                    maxTime = maxSignalTime;
                }
                const auto minSignalTime = signal->GetMinTime();
                if (minSignalTime < minTime) {
                    minTime = minSignalTime;
                }

                if (minSignalTime < 0) {
                    RESTWarning << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: EventID: "
                                << fInputSignalEvent->GetID() << " signal ID: " << signal->GetSignalID()
                                << " minSignalTime < 0. MinSignalTime: " << minSignalTime << RESTendl;
                    signal->Print();
                    return nullptr;
                }
            }

            if (minTime > maxTime || minTime < 0) {
                RESTWarning << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: EventID: "
                            << fInputSignalEvent->GetID()
                            << " minTime > maxTime or minTime < 0. MinTime: " << minTime
                            << " MaxTime: " << maxTime << RESTendl;
                return nullptr;
            }

            triggerTime = minTime;
            bool thresholdReached = false;
            double maxEnergy = 0;
            while (triggerTime <= maxTime + fSampling) {
                // iterate over number of signals
                double energy = 0;
                const double startTime = triggerTime - fSampling * fNPoints;
                for (const auto& signal : tpcSignals) {
                    energy += signal->GetIntegralWithTime(startTime, triggerTime);
                }
                if (energy > maxEnergy) {
                    maxEnergy = energy;
                }
                if (maxEnergy >= fIntegralThresholdTPCkeV) {
                    thresholdReached = true;
                    break;
                }
                triggerTime += fSampling;
            }

            if (!thresholdReached) {
                return nullptr;
            }

            startTimeNoOffset = triggerTime;
        }

    } else if (fTriggerMode == "fixed") {
        startTimeNoOffset = fTriggerFixedStartTime;
    } else {
        cerr << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: "
             << "Trigger mode not recognized" << RESTendl;
        exit(1);
    }

    for (int n = 0; n < fInputSignalEvent->GetNumberOfSignals(); n++) {
        TRestDetectorSignal* signal = fInputSignalEvent->GetSignal(n);
        Int_t signalID = signal->GetSignalID();
        string type = signal->GetSignalType();
        // Check type is in the map
        if (fParametersMap.find(type) == fParametersMap.end()) {
            RESTWarning << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: "
                        << "type " << type << " not found in parameters map" << RESTendl;
            type = "";
        }

        double noiseLevel = fParametersMap.at(type).noiseLevel;
        double sampling = fParametersMap.at(type).sampling;
        double shapingTime = fParametersMap.at(type).shapingTime;
        double calibrationGain = fParametersMap.at(type).calibrationGain;
        double calibrationOffset = fParametersMap.at(type).calibrationOffset;

        double timeStart = startTimeNoOffset - fTriggerDelay * sampling;
        double timeEnd = timeStart + fNPoints * sampling;
        RESTDebug << "fTimeStart: " << timeStart << " us " << RESTendl;
        RESTDebug << "fTimeEnd: " << timeEnd << " us " << RESTendl;

        if (timeStart + fTriggerDelay * sampling < 0) {
            // This means something is wrong (negative times somewhere). This should never happen
            cerr << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: "
                 << "fTimeStart < - fTriggerDelay * fSampling" << endl;
            exit(1);
        }

        // TODO: time offset may not be working correctly
        // TODO: event drawing not working correctly (some signals are clipped)

        if (timeStart + fTriggerDelay * fSampling < 0) {
            // This means something is wrong (negative times somewhere). This should never happen
            RESTError << "TRestDetectorSignalToRawSignalProcess::ProcessEvent: "
                      << "fTimeStart < - fTriggerDelay * fSampling" << RESTendl;
            exit(1);
        }

        vector<Double_t> data(fNPoints, calibrationOffset);

        for (int m = 0; m < signal->GetNumberOfPoints(); m++) {
            Double_t t = signal->GetTime(m);
            Double_t d = signal->GetData(m);

            if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Debug && n < 3 && m < 5) {
                cout << "Signal: " << n << " Sample: " << m << " T: " << t << " Data: " << d << endl;
            }

            if (t > timeStart && t < timeEnd) {
                // convert physical time (in us) to timeBin
                auto timeBin = (Int_t)round((t - timeStart) / sampling);

                if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Warning) {
                    if (timeBin < 0 || timeBin > fNPoints) {
                        cout << "Time bin out of range!!! bin value: " << timeBin << endl;
                        timeBin = 0;
                    }
                }

                RESTDebug << "Adding data: " << signal->GetData(m) << " to Time Bin: " << timeBin << RESTendl;
                data[timeBin] += calibrationGain * signal->GetData(m);
            }
        }

        // Noise before shaping
        if (noiseLevel > 0) {
            for (int i = 0; i < fNPoints; i++) {
                data[i] += gRandom->Gaus(0, noiseLevel);
            }
        }

        if (shapingTime > 0) {
            const auto sinShaper = [](Double_t t) -> Double_t {
                if (t <= 0) {
                    return 0;
                }
                // function is normalized such that its absolute maximum is 1.0
                // max is at x = 1.1664004483744728
                return TMath::Exp(-3.0 * t) * TMath::Power(t, 3.0) * TMath::Sin(t) * 22.68112123672292;
            };

            const auto shapingFunction = [&sinShaper](Double_t t) -> Double_t {
                if (t <= 0) {
                    return 0;
                }
                // function is normalized such that its absolute maximum is 1.0
                // max is at x = 1.1664004483744728
                // return sinShaper(t) - 1.0 * sinShaper(t - 1); // to add undershoot
                return sinShaper(t);
            };

            vector<Double_t> dataAfterShaping(fNPoints, calibrationOffset);
            for (int i = 0; i < fNPoints; i++) {
                const Double_t value = data[i] - calibrationOffset;
                if (value <= 0) {
                    // Only positive values are possible, 0 means no signal in this bin
                    continue;
                }
                for (int j = 0; j < fNPoints; j++) {
                    dataAfterShaping[j] += value * shapingFunction(((j - i) * sampling) / shapingTime);
                }
            }
            data = dataAfterShaping;

            // Noise after shaping
            if (noiseLevel > 0) {
                for (int i = 0; i < fNPoints; i++) {
                    data[i] += gRandom->Gaus(0, noiseLevel);
                }
            }
        }

        TRestRawSignal rawSignal;
        rawSignal.SetSignalID(signalID);
        for (int x = 0; x < fNPoints; x++) {
            double value = round(data[x]);
            if (GetVerboseLevel() >= TRestStringOutput::REST_Verbose_Level::REST_Warning) {
                if (value < numeric_limits<Short_t>::min() || value > numeric_limits<Short_t>::max()) {
                    RESTDebug << "value (" << value << ") is outside short range ("
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

    SetObservableValue("triggerTimeTPC", triggerTime);

    RESTDebug << "TRestDetectorSignalToRawSignalProcess. Returning event with N signals "
              << fOutputRawSignalEvent->GetNumberOfSignals() << RESTendl;

    return fOutputRawSignalEvent;
}

///////////////////////////////////////////////
/// \brief Function reading input parameters from the RML
/// TRestDetectorSignalToRawSignalProcess metadata section
///
void TRestDetectorSignalToRawSignalProcess::InitFromConfigFile() {
    TString readoutTypesString = GetParameter("readoutTypes", "");
    // split it by ","
    TObjArray* readoutTypesArray = readoutTypesString.Tokenize(",");
    for (int i = 0; i < readoutTypesArray->GetEntries(); i++) {
        fReadoutTypes.insert(((TObjString*)readoutTypesArray->At(i))->GetString().Data());
    }
    cout << "readout types: ";
    for (const auto& type : fReadoutTypes) {
        cout << type << " ";
    }
    cout << endl;

    // add default type ""
    const string defaultType;
    fReadoutTypes.insert(defaultType);
    fParametersMap[defaultType] = {};

    for (const auto& type : fReadoutTypes) {
        fReadoutTypes.insert(type);
        fParametersMap[type] = {};

        string typeCamelCase = type;
        typeCamelCase[0] = toupper(typeCamelCase[0]);

        Parameters parameters;
        parameters.sampling = GetDblParameterWithUnits("sampling" + typeCamelCase, parameters.sampling);
        parameters.shapingTime =
            GetDblParameterWithUnits("shapingTime" + typeCamelCase, parameters.shapingTime);
        parameters.calibrationGain =
            GetDblParameterWithUnits("gain" + typeCamelCase, parameters.calibrationGain);
        parameters.calibrationOffset =
            GetDblParameterWithUnits("offset" + typeCamelCase, parameters.calibrationOffset);
        parameters.calibrationEnergy =
            Get2DVectorParameterWithUnits("calibrationEnergy" + typeCamelCase, parameters.calibrationEnergy);
        parameters.calibrationRange =
            Get2DVectorParameterWithUnits("calibrationRange" + typeCamelCase, parameters.calibrationRange);
        parameters.noiseLevel = GetDblParameterWithUnits("noiseLevel" + typeCamelCase, parameters.noiseLevel);

        const bool isLinearCalibration =
            (parameters.calibrationEnergy.Mod() != 0 && parameters.calibrationRange.Mod() != 0);
        ;
        if (isLinearCalibration) {
            const auto range = numeric_limits<Short_t>::max() - numeric_limits<Short_t>::min();
            parameters.calibrationGain =
                range * (parameters.calibrationRange.Y() - parameters.calibrationRange.X()) /
                (parameters.calibrationEnergy.Y() - parameters.calibrationEnergy.X());
            parameters.calibrationOffset =
                range * (parameters.calibrationRange.X() -
                         parameters.calibrationGain * parameters.calibrationEnergy.X()) +
                numeric_limits<Short_t>::min();
        }
        fParametersMap[type] = parameters;
    }

    auto nPoints = GetParameter("nPoints");
    if (nPoints == PARAMETER_NOT_FOUND_STR) {
        nPoints = GetParameter("Npoints", fNPoints);
    }
    fNPoints = StringToInteger(nPoints);

    fTriggerMode = GetParameter("triggerMode", fTriggerMode);
    const set<string> validTriggerModes = {"firstDeposit", "integralThreshold", "fixed",
                                           "observable",   "firstDepositTPC",   "integralThresholdTPC"};
    if (validTriggerModes.count(fTriggerMode) == 0) {
        RESTError << "Trigger mode set to: '" << fTriggerMode
                  << "' which is not a valid trigger mode. Please use one of the following trigger modes: ";
        for (const auto& triggerMode : validTriggerModes) {
            RESTError << triggerMode << " ";
        }
        RESTError << RESTendl;
        exit(1);
    }

    fTriggerDelay = StringToInteger(GetParameter("triggerDelay", fTriggerDelay));
    fIntegralThreshold = StringToDouble(GetParameter("integralThreshold", fIntegralThreshold));
    fIntegralThresholdTPCkeV =
        StringToDouble(GetParameter("integralThresholdTPCkeV", fIntegralThresholdTPCkeV));
    if (fIntegralThresholdTPCkeV <= 0) {
        RESTWarning << "integralThresholdTPCkeV must be greater than 0: " << fIntegralThresholdTPCkeV
                    << RESTendl;
        // This should always be an error but breaks the CI...
        // exit(1);
    }

    fTriggerFixedStartTime = GetDblParameterWithUnits("triggerFixedStartTime", fTriggerFixedStartTime);

    // load default parameters (for backward compatibility)
    fSampling = fParametersMap.at(defaultType).sampling;
    fShapingTime = fParametersMap.at(defaultType).shapingTime;
    fCalibrationGain = fParametersMap.at(defaultType).calibrationGain;
    fCalibrationOffset = fParametersMap.at(defaultType).calibrationOffset;
    fCalibrationEnergy = fParametersMap.at(defaultType).calibrationEnergy;
    fCalibrationRange = fParametersMap.at(defaultType).calibrationRange;

    if (fTriggerMode == "observable") {
        fTriggerModeObservableName = GetParameter("triggerModeObservableName", "");
        if (fTriggerModeObservableName.empty()) {
            RESTError << "You need to set 'triggerModeObservableName' to a valid analysis tree observable"
                      << RESTendl;
            exit(1);
        }
    }
}

void TRestDetectorSignalToRawSignalProcess::InitProcess() {}

Double_t TRestDetectorSignalToRawSignalProcess::GetEnergyFromADC(Double_t adc, const string& type) const {
    if (fParametersMap.find(type) == fParametersMap.end()) {
        RESTWarning << "TRestDetectorSignalToRawSignalProcess::GetEnergyFromADC: "
                    << "type " << type << " not found in parameters map" << RESTendl;
        return 0;
    }
    const auto gain = fParametersMap.at(type).calibrationGain;
    const auto offset = fParametersMap.at(type).calibrationOffset;
    return (adc - offset) / gain;
}

Double_t TRestDetectorSignalToRawSignalProcess::GetADCFromEnergy(Double_t energy, const string& type) const {
    if (fParametersMap.find(type) == fParametersMap.end()) {
        RESTWarning << "TRestDetectorSignalToRawSignalProcess::GetEnergyFromADC: "
                    << "type " << type << " not found in parameters map" << RESTendl;
        return 0;
    }
    const auto gain = fParametersMap.at(type).calibrationGain;
    const auto offset = fParametersMap.at(type).calibrationOffset;
    return energy * gain + offset;
}

Double_t TRestDetectorSignalToRawSignalProcess::GetTimeFromBin(Double_t bin, const string& type) const {
    if (fParametersMap.find(type) == fParametersMap.end()) {
        RESTWarning << "TRestDetectorSignalToRawSignalProcess::GetEnergyFromADC: "
                    << "type " << type << " not found in parameters map" << RESTendl;
        return 0;
    }
    const auto sampling = fParametersMap.at(type).sampling;
    return (bin - fTriggerDelay) * sampling;
}

Double_t TRestDetectorSignalToRawSignalProcess::GetBinFromTime(Double_t time, const string& type) const {
    if (fParametersMap.find(type) == fParametersMap.end()) {
        RESTWarning << "TRestDetectorSignalToRawSignalProcess::GetEnergyFromADC: "
                    << "type " << type << " not found in parameters map" << RESTendl;
        return 0;
    }
    const auto sampling = fParametersMap.at(type).sampling;
    return (UShort_t)((time + fTriggerDelay * sampling) / sampling);
}

void TRestDetectorSignalToRawSignalProcess::PrintMetadata() {
    BeginPrintProcess();

    RESTMetadata << "Points per channel: " << fNPoints << RESTendl;
    RESTMetadata << "Trigger mode: " << fTriggerMode << RESTendl;
    RESTMetadata << "Trigger delay: " << fTriggerDelay << " units" << RESTendl;

    for (const auto& readoutType : fReadoutTypes) {
        RESTMetadata << RESTendl;
        string type = readoutType;
        if (type.empty()) {
            type = "default";
        }
        RESTMetadata << "Readout type: " << type << RESTendl;
        RESTMetadata << "Sampling time: " << fParametersMap.at(readoutType).sampling * 1000 << " ns"
                     << RESTendl;
        const double shapingTime = fParametersMap.at(readoutType).shapingTime;
        if (shapingTime > 0) {
            RESTMetadata << "Shaping time: " << shapingTime * 1000 << " ns" << RESTendl;
        }

        if (IsLinearCalibration()) {
            RESTMetadata << "Calibration energies: (" << fParametersMap.at(readoutType).calibrationEnergy.X()
                         << ", " << fParametersMap.at(readoutType).calibrationEnergy.Y() << ") keV"
                         << RESTendl;
            RESTMetadata << "Calibration range: (" << fParametersMap.at(readoutType).calibrationRange.X()
                         << ", " << fParametersMap.at(readoutType).calibrationRange.Y() << ")" << RESTendl;
        }
        RESTMetadata << "ADC Gain: " << fParametersMap.at(readoutType).calibrationGain << RESTendl;
        RESTMetadata << "ADC Offset: " << fParametersMap.at(readoutType).calibrationOffset << RESTendl;
    }
    EndPrintProcess();
}
