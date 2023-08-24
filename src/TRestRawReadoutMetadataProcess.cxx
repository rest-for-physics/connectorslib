//
// Created by lobis on 24-Aug-23.
//

#include "TRestRawReadoutMetadataProcess.h"

#include <TRestDetectorReadout.h>

#include "TRestRawReadoutMetadata.h"

using namespace std;

ClassImp(TRestRawReadoutMetadataProcess);

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
                fChannelInfo[channel->GetChannelId()] = info;
            }
        }
    }
}

void TRestRawReadoutMetadataProcess::InitProcess() {
    fReadout = GetMetadata<TRestDetectorReadout>();
    if (!fReadout) {
        cerr << "TRestRawReadoutMetadataProcess::InitProcess: readout is null" << endl;
        exit(1);
    }

    TRestRawReadoutMetadata* metadata = GetMetadata<TRestRawReadoutMetadata>();
    if (!metadata) {
        metadata = new TRestRawReadoutMetadata();

        metadata->InitializeFromReadout(fReadout);

        metadata->Write("readoutRawMetadata", TObject::kOverwrite);
    }
}