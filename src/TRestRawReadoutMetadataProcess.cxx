//
// Created by lobis on 24-Aug-23.
//

#include "TRestRawReadoutMetadataProcess.h"

#include <TRestDetectorReadout.h>

#include <set>

#include "TRestRawReadoutMetadata.h"

using namespace std;

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

void TRestRawReadoutMetadata::PrintMetadata() const {
    cout << "Number of channels: " << fChannelInfo.size() << endl;
    map<string, int> typesCount;
    for (const auto& channel : fChannelInfo) {
        const auto& info = channel.second;
        typesCount[info.type]++;
    }
    cout << "Channel types:" << endl;
    for (const auto& type : typesCount) {
        cout << type.first << ": " << type.second << endl;
    }

    for (const auto& [channelId, info] : fChannelInfo) {
        cout << "Channel " << channelId << ": " << info.type << " " << info.name << endl;
    }
}
