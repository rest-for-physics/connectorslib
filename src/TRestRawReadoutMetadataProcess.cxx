//
// Created by lobis on 24-Aug-23.
//

#include "TRestRawReadoutMetadataProcess.h"

#include <TRestDetectorReadout.h>
#include <TRestRawPeaksFinderProcess.h>
#include <TRestRawReadoutMetadata.h>
#include <TRestRawSignalAnalysisProcess.h>

#include <mutex>

using namespace std;

ClassImp(TRestRawReadoutMetadataProcess);

TRestRawReadoutMetadata* TRestRawReadoutMetadataProcess::fReadoutMetadata = nullptr;

mutex TRestRawReadoutMetadataProcess::fMetadataMutex = {};

void TRestRawReadoutMetadata::InitializeFromReadout(TRestDetectorReadout* readout) {
    if (!readout) {
        cerr << "TRestRawReadoutMetadata::InitializeFromReadout: readout is null" << endl;
        exit(1);
    }
    fChannelInfo.clear();
    for (int planeIndex = 0; planeIndex < readout->GetNumberOfReadoutPlanes(); planeIndex++) {
        const auto plane = readout->GetReadoutPlane(planeIndex);
        for (unsigned int moduleIndex = 0; moduleIndex < plane->GetNumberOfModules(); moduleIndex++) {
            const auto module = plane->GetModule(moduleIndex);
            for (unsigned int channelIndex = 0; channelIndex < module->GetNumberOfChannels();
                 channelIndex++) {
                const auto channel = module->GetChannel(channelIndex);
                const UShort_t channelId = channel->GetChannelId();
                // check if channel id is already in the map
                if (fChannelInfo.find(channelId) != fChannelInfo.end()) {
                    cerr << "TRestRawReadoutMetadata::InitializeFromReadout: channel id " << channelId
                         << " already in the map. Channels on the readout should be unique" << endl;
                    exit(1);
                }
                ChannelInfo info;
                info.type = channel->GetChannelType();
                info.name = channel->GetChannelName();
                info.daqId = channel->GetDaqID();
                if (info.name.empty()) {
                    info.name = "daqid" + to_string(info.daqId);
                }

                fChannelInfo[channel->GetChannelId()] = info;
            }
        }
    }

    // verify names are unique
    map<string, int> namesCount;
    for (const auto& channel : fChannelInfo) {
        const auto& info = channel.second;
        namesCount[info.name]++;
    }
    for (const auto& name : namesCount) {
        if (name.second > 1) {
            cerr << "TRestRawReadoutMetadata::InitializeFromReadout: channel name " << name.first
                 << " is not unique" << endl;
            exit(1);
        }
    }
}

Int_t TRestRawReadoutMetadata::GetDaqIdForChannelId(UShort_t channel) const {
    if (fChannelInfo.find(channel) == fChannelInfo.end()) {
        return -1;
    }
    return fChannelInfo.at(channel).daqId;
}

void TRestRawReadoutMetadataProcess::InitProcess() {
    fReadout = GetMetadata<TRestDetectorReadout>();
    if (!fReadout) {
        cerr << "TRestRawReadoutMetadataProcess::InitProcess: readout is null" << endl;
        exit(1);
    }

    std::lock_guard<std::mutex> lock(fMetadataMutex);

    if (!fReadoutMetadata) {
        fReadoutMetadata = GetMetadata<TRestRawReadoutMetadata>();
        if (!fReadoutMetadata) {
            fReadoutMetadata = new TRestRawReadoutMetadata();
            fReadoutMetadata->InitializeFromReadout(fReadout);
        }

        fReadoutMetadata->Write("readoutRawMetadata", TObject::kOverwrite);
        // write only if it's the main thread
    }

    TRestRawPeaksFinderProcess::Metadata = fReadoutMetadata;
    TRestRawSignalAnalysisProcess::Metadata = fReadoutMetadata;
}

TRestEvent* TRestRawReadoutMetadataProcess::ProcessEvent(TRestEvent* inputEvent) {
    fSignalEvent = dynamic_cast<TRestRawSignalEvent*>(inputEvent);

    return inputEvent;
}
