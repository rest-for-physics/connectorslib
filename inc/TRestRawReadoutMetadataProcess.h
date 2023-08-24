//
// Created by lobis on 24-Aug-23.
//

#ifndef REST_TRESTRAWREADOUTMETADATAPROCESS_H
#define REST_TRESTRAWREADOUTMETADATAPROCESS_H

#include <TRestDetectorReadout.h>
#include <TRestEventProcess.h>
#include <TRestRawReadoutMetadata.h>
#include <TRestRawSignalEvent.h>

class TRestRawReadoutMetadataProcess : public TRestEventProcess {
   private:
    TRestRawSignalEvent* fSignalEvent;  //!
    TRestDetectorReadout* fReadout;     //!

   public:
    any GetInputEvent() const override { return fSignalEvent; }
    any GetOutputEvent() const override { return fSignalEvent; }

    void InitProcess() override;

    const char* GetProcessName() const override { return "readoutMetadata"; }

    TRestRawReadoutMetadataProcess() = default;
    ~TRestRawReadoutMetadataProcess() = default;

    ClassDefOverride(TRestRawReadoutMetadataProcess, 1);
};

#endif  // REST_TRESTRAWREADOUTMETADATAPROCESS_H
