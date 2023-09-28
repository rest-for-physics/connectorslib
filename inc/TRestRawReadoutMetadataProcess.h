//
// Created by lobis on 24-Aug-23.
//

#ifndef REST_TRESTRAWREADOUTMETADATAPROCESS_H
#define REST_TRESTRAWREADOUTMETADATAPROCESS_H

#include <TRestDetectorReadout.h>
#include <TRestEventProcess.h>
#include <TRestRawReadoutMetadata.h>
#include <TRestRawSignalEvent.h>

#include <atomic>

class TRestRawReadoutMetadataProcess : public TRestEventProcess {
   private:
    TRestRawSignalEvent* fSignalEvent = nullptr;  //!
    TRestDetectorReadout* fReadout;               //!
   private:
    static std::mutex fMetadataMutex;  //!

   public:
    RESTValue GetInputEvent() const override { return fSignalEvent; }
    RESTValue GetOutputEvent() const override { return fSignalEvent; }

    void InitProcess() override;
    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;
    void EndProcess() override {}

    const char* GetProcessName() const override { return "readoutMetadata"; }

    explicit TRestRawReadoutMetadataProcess(const char* configFilename){};

    TRestRawReadoutMetadataProcess() = default;
    ~TRestRawReadoutMetadataProcess() = default;

    // this is a workaround
    static TRestRawReadoutMetadata* fReadoutMetadata;  //! // made static to avoid problems with MT

    ClassDefOverride(TRestRawReadoutMetadataProcess, 1);
};

#endif  // REST_TRESTRAWREADOUTMETADATAPROCESS_H
