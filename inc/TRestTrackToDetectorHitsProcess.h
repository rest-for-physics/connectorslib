//////////////////////////////////////////////////////////////////////////
///
///
///             RESTSoft : Software for Rare Event Searches with TPCs
///
///             TRestTrackToDetectorHitsProcess.h
///
///             Apr 2017 : Javier Galan
///
//////////////////////////////////////////////////////////////////////////

#ifndef RestCore_TRestTrackToDetectorHitsProcess
#define RestCore_TRestTrackToDetectorHitsProcess

#include <TRestDetectorHitsEvent.h>
#include <TRestTrackEvent.h>

#include "TRestEventProcess.h"

class TRestTrackToDetectorHitsProcess : public TRestEventProcess {
   private:
#ifndef __CINT__
    TRestTrackEvent* fInputTrackEvent;         //!
    TRestDetectorHitsEvent* fOutputHitsEvent;  //!
#endif

    void InitFromConfigFile() override;

    void Initialize() override;

   protected:
    Int_t fTrackLevel;

   public:
    RESTValue GetInputEvent() const override { return fInputTrackEvent; }
    RESTValue GetOutputEvent() const override { return fOutputHitsEvent; }

    void InitProcess() override;
    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;
    void EndProcess() override;
    void LoadDefaultConfig();

    void LoadConfig(const std::string& configFilename, const std::string& name = "");

    void PrintMetadata() override {
        BeginPrintProcess();

        std::cout << "Track level : " << fTrackLevel << std::endl;

        EndPrintProcess();
    }

    const char* GetProcessName() const override { return "trackToDetectorHits"; }

    // Constructor
    TRestTrackToDetectorHitsProcess();
    TRestTrackToDetectorHitsProcess(const char* configFilename);
    // Destructor
    ~TRestTrackToDetectorHitsProcess();

    ClassDefOverride(TRestTrackToDetectorHitsProcess, 1);  // Template for a REST "event process" class
                                                           // inherited from TRestEventProcess
};
#endif
