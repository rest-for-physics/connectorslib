///______________________________________________________________________________
///______________________________________________________________________________
///______________________________________________________________________________
///
///
///             RESTSoft : Software for Rare Event Searches with TPCs
///
///             TRestDetectorHitsToTrackFastProcess.h
///
///_______________________________________________________________________________

#ifndef RestCore_TRestDetectorHitsToTrackFastProcess
#define RestCore_TRestDetectorHitsToTrackFastProcess

#include <TRestDetectorHitsEvent.h>
#include <TRestEventProcess.h>
#include <TRestTrackEvent.h>
#include <TVector3.h>

class TRestDetectorHitsToTrackFastProcess : public TRestEventProcess {
   private:
#ifndef __CINT__
    TRestDetectorHitsEvent* fHitsEvent;  //!
    TRestTrackEvent* fTrackEvent;        //!
#endif

    void InitFromConfigFile() override;

    void Initialize() override;
    Int_t FindTracks(TRestHits* hits);

   protected:
    // add here the members of your event process

    Double_t fCellResolution;
    Double_t fNetSize;
    TVector3 fNetOrigin;
    Int_t fNodes;

   public:
    any GetInputEvent() const override { return fHitsEvent; }
    any GetOutputEvent() const override { return fTrackEvent; }

    void InitProcess() override;
    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;
    void EndProcess() override;
    void LoadDefaultConfig();

    void LoadConfig(const std::string& configFilename, const std::string& name = "");

    void PrintMetadata() override {
        BeginPrintProcess();

        RESTMetadata << " Cell resolution : " << fCellResolution << " mm " << RESTendl;
        RESTMetadata << " Net size : " << fNetSize << " mm " << RESTendl;
        RESTMetadata << " Net origin : ( " << fNetOrigin.X() << " , " << fNetOrigin.Y() << " , " << fNetOrigin.Z()
                 << " ) mm " << RESTendl;
        RESTMetadata << " Number of nodes (per axis) : " << fNodes << RESTendl;

        EndPrintProcess();
    }

    const char* GetProcessName() const override { return "fastHitsToTrack"; }

    // Constructor
    TRestDetectorHitsToTrackFastProcess();
    TRestDetectorHitsToTrackFastProcess(const char* configFilename);
    // Destructor
    ~TRestDetectorHitsToTrackFastProcess();

    ClassDefOverride(TRestDetectorHitsToTrackFastProcess,
                     1);  // Template for a REST "event process" class inherited from
                          // TRestEventProcess
};
#endif
