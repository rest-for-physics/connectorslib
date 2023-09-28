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
/// The TRestRawToDetectorSignalProcess transforms a TRestRawSignalEvent into
/// a TRestDetectorSignalEvent. It applies a direct transform between both data
/// types. The data points inside the raw signal are transformed to time
/// information using the input sampling time and time start provided
/// through the process RML section. A new method for zero suppression has been
/// implemented, now it is capable to perform zero suppression as it was implemented
/// int the TRestRawZeroSuppresionProcess, which identifies the points that are over
/// threshold from the input TRestRawSignalEvent. The resulting points remains as a
/// TRestRawSignalEvent.
///
/// All the data points will be transferred to the output signal event.
///
/// The following list describes the different parameters that can be
/// used in this process.
/// * **sampling**: It is the sampling time of input raw signal data.
/// Time units must be specified (ns, us, ms).
/// * **triggerStarts**: It defines the physical time value for the first
/// bin for the input raw signal data.
/// * **gain**: Each data point from the resulting output signal will be
/// multiplied by this factor.
/// * **threshold**: Minimum threshold required to add the raw signal data
/// into de the detector data.
/// * **zeroSuppression**: If true, performs zero suppression of the data
/// * **baselineRange**: A 2D-vector definning the range, in number of bins,
/// where the baseline properties will be calculated.
/// * **integralRange**: A 2D-vector definning the time window, in number of bins,
/// where the signal will be considered.
/// * **pointThreshold**: The number of sigmas over the baseline flunctuations to
/// consider a point is over the threshold.
/// * **pointsOverThreshold**: The number of consecutive points over threshold
/// required to consider them as a physical signal.
/// * **signalThreshold**: The number of sigmas a set of consecutive points
/// identified over threshold must be over the baseline fluctuations to be
/// finally considered a physical signal.
///
/// List of observables:
///
/// * NSignalsRejected: Number of rejected signals inside a event, due to
/// zero suppression or just because it is below the desired threshold.
///
/// The following lines of code show how the process metadata should be
/// defined.
///
/// \code
///
/// // A raw signal with 200ns binning will be translated to a
/// // TRestDetectorSignalEvent. The new signal will start at time=20us, and its
/// // amplitude will be reduced a factor 50. If zeroSuppression is true it will
/// // perform
///
/// <TRestRawToDetectorSignalProcess name="rsTos" title"Raw signal to signal">
///     <parameter name="sampling" value="0.2" units="us" />
///     <parameter name="triggerStarts" value="20" units="us" />
///     <parameter name="gain" value="1/50." />
///     <parameter name="zeroSuppression" value="true"/>
///     <parameter name="baseLineRange" value="(20,140)"/>
///     <parameter name="integralRange" value="(150,450)"/>
///     <parameter name="pointThreshold" value="3"/>
///     <parameter name="signalThreshold" value="7"/>
///     <parameter name="nPointsOverThreshold" value="7"/>
///     <observable name="NSignalsRejected" value="ON"/>
///
/// </TRestRawToDetectorSignalProcess>
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
/// 2016-February: First implementation of rawsignal to signal conversion.
///             Javier Gracia
///
/// 2017-November: Class documented and re-furbished
/// 2022-January: Added threshold parameter
///             Javier Galan
/// 2022-January: Adding ZeroSuppression method
///             JuanAn Garcia
///
/// \class      TRestRawToDetectorSignalProcess
/// \author     Javier Gracia
/// \author     Javier Galan
///
///
/// <hr>
///

#include "TRestRawToDetectorSignalProcess.h"

using namespace std;

ClassImp(TRestRawToDetectorSignalProcess);

///////////////////////////////////////////////
/// \brief Default constructor
///
TRestRawToDetectorSignalProcess::TRestRawToDetectorSignalProcess() { Initialize(); }

///////////////////////////////////////////////
/// \brief Default destructor
///
TRestRawToDetectorSignalProcess::~TRestRawToDetectorSignalProcess() { delete fOutputSignalEvent; }

///////////////////////////////////////////////
/// \brief Function to initialize input/output event members and define the
/// section name
///
void TRestRawToDetectorSignalProcess::Initialize() {
    SetSectionName(this->ClassName());
    SetLibraryVersion(LIBRARY_VERSION);

    fInputSignalEvent = nullptr;
    fOutputSignalEvent = new TRestDetectorSignalEvent();
}

///////////////////////////////////////////////
/// \brief The main processing event function
///
TRestEvent* TRestRawToDetectorSignalProcess::ProcessEvent(TRestEvent* inputEvent) {
    fInputSignalEvent = (TRestRawSignalEvent*)inputEvent;

    Int_t rejectedSignal = 0;

    if (fZeroSuppression) {
        fInputSignalEvent->SetBaseLineRange(fBaseLineRange);
        fInputSignalEvent->SetRange(fIntegralRange);
    }

    for (int n = 0; n < fInputSignalEvent->GetNumberOfSignals(); n++) {
        TRestDetectorSignal signal;
        TRestRawSignal* rawSignal = fInputSignalEvent->GetSignal(n);
        signal.SetID(rawSignal->GetID());

        if (fZeroSuppression) {
            ZeroSuppresion(rawSignal, signal);
        } else {
            for (int p = 0; p < int(rawSignal->GetNumberOfPoints()); p++) {
                if (rawSignal->GetData(p) > fThreshold) {
                    signal.NewPoint(fTriggerStarts + fSampling * p, fGain * rawSignal->GetData(p));
                }
            }
        }

        if (signal.GetNumberOfPoints() > 0) {
            fOutputSignalEvent->AddSignal(signal);
        } else {
            rejectedSignal++;
        }
    }

    SetObservableValue("NSignalsRejected", rejectedSignal);

    if (fOutputSignalEvent->GetNumberOfSignals() <= 0) {
        return nullptr;
    }

    return fOutputSignalEvent;
}

void TRestRawToDetectorSignalProcess::ZeroSuppresion(TRestRawSignal* rawSignal, TRestDetectorSignal& signal) {
    rawSignal->InitializePointsOverThreshold(TVector2(fPointThreshold, fSignalThreshold),
                                             fNPointsOverThreshold, 512);

    std::vector<Int_t> pOver = rawSignal->GetPointsOverThreshold();
    for (unsigned int n = 0; n < pOver.size(); n++) {
        int j = pOver[n];
        signal.NewPoint(fTriggerStarts + fSampling * j, fGain * rawSignal->GetData(j));
    }
}
