//
// Created by lobis on 10/19/2021.
//

#ifndef REST_TRESTGEANT4TOVETOSIGNALPROCESS_H
#define REST_TRESTGEANT4TOVETOSIGNALPROCESS_H

#include <TRestDetectorSignalEvent.h>
#include <TRestEventProcess.h>
#include <TRestGeant4DataEvent.h>
#include <TRestGeant4Event.h>
#include <TRestGeant4Metadata.h>

class TRestGeant4Event;
class TRestGeant4Metadata;
class TRestDetectorSignalEvent;

class TRestGeant4ToVetoSignalProcess : public TRestEventProcess {
   private:
    TString fProcessName = "Geant4ToVetoSignalEvent";

    /// A pointer to the input TRestGeant4Event
    TRestGeant4Event* fRestGeant4Event;  //!

    /// A pointer to the Geant4 simulation conditions stored in TRestGeant4Metadata
    TRestGeant4Metadata* fRestGeant4Metadata;  //!

    /// A pointer to the output TRestSignalEvent
    TRestDetectorSignalEvent* fRestDetectorSignalEvent;  //!

    void InitFromConfigFile() override;

    void Initialize() override;

   protected:
   public:
    TRestGeant4ToVetoSignalProcess();
    ~TRestGeant4ToVetoSignalProcess();

    explicit TRestGeant4ToVetoSignalProcess(char* rmlFile);

    void InitProcess() override;

    any GetInputEvent() override { return fRestGeant4Event; }
    any GetOutputEvent() override { return fRestDetectorSignalEvent; }

    void BeginOfEventProcess(TRestEvent*) override;

    TRestEvent* ProcessEvent(TRestEvent*) override;

    void EndOfEventProcess(TRestEvent*) override;

    void EndProcess() override;

    void PrintMetadata() const override;

    /// Returns the name of this process
    TString GetProcessName() const { return fProcessName; }

    ClassDef(TRestGeant4ToVetoSignalProcess, 1);
};
#endif  // REST_TRESTGEANT4TOVETOSIGNALPROCESS_H
