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

    void InitFromConfigFile();

    void Initialize();

   protected:
    Int_t fTrackLevel;

   public:
    inline any GetInputEvent() const { return fInputTrackEvent; }
    inline any GetOutputEvent() const { return fOutputHitsEvent; }

    void InitProcess();
    TRestEvent* ProcessEvent(TRestEvent* eventInput);
    void EndProcess();
    void LoadDefaultConfig();

    void LoadConfig(std::string configFilename, std::string name = "");

    void PrintMetadata() {
        BeginPrintProcess();

        std::cout << "Track level : " << fTrackLevel << endl;

        EndPrintProcess();
    }

    inline TString GetProcessName() const { return (TString) "trackToDetectorHits"; }

    // Constructor
    TRestTrackToDetectorHitsProcess();
    TRestTrackToDetectorHitsProcess(char* configFilename);
    // Destructor
    ~TRestTrackToDetectorHitsProcess();

    ClassDef(TRestTrackToDetectorHitsProcess, 1);  // Template for a REST "event process" class inherited from
                                                   // TRestEventProcess
};
#endif
