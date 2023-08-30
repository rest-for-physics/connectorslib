///______________________________________________________________________________
///______________________________________________________________________________
///______________________________________________________________________________
///
///
///             RESTSoft : Software for Rare Event Searches with TPCs
///
///             TRestRawReadoutAnalysisProcess.h
///
///_______________________________________________________________________________

#ifndef RestCore_TRestRawReadoutAnalysisProcess
#define RestCore_TRestRawReadoutAnalysisProcess

#include <TH1D.h>

// #include <TCanvas.h>

#include <TRestDetectorGas.h>
#include <TRestDetectorHitsEvent.h>
#include <TRestDetectorReadout.h>
#include <TRestRawSignalEvent.h>

#include "TRestEventProcess.h"

class TRestRawReadoutAnalysisProcess : public TRestEventProcess {
   private:
#ifndef __CINT__
    TRestRawSignalEvent* fSignalEvent;  //!

    TRestDetectorReadout* fReadout;  //!

#endif

    void InitFromConfigFile() override;

    void Initialize() override;

    std::string fModuleCanvasSave;  //!

    // plots (saved directly in root file)
    std::map<int, TH2D*> fModuleHitMaps;    //! [MM id, channel activity]
    std::map<int, TH1D*> fModuleActivityX;  //! [MM id, channel activity]
    std::map<int, TH1D*> fModuleActivityY;  //! [MM id, channel activity]
    std::map<int, TH2D*> fModuleBSLSigmaX;  //! [MM id, channel activity]
    std::map<int, TH2D*> fModuleBSLSigmaY;  //! [MM id, channel activity]
                                            //

   public:
    RESTValue GetInputEvent() const override { return fSignalEvent; }
    RESTValue GetOutputEvent() const override { return fSignalEvent; }

    void InitProcess() override;
    TRestEvent* ProcessEvent(TRestEvent* inputEvent) override;
    void EndProcess() override;

    void PrintMetadata() override {
        BeginPrintProcess();

        RESTMetadata << "channel activity and hitmap histograms required for module: ";
        auto iter2 = fModuleHitMaps.begin();
        while (iter2 != fModuleHitMaps.end()) {
            RESTMetadata << iter2->first << ", ";
            iter2++;
        }
        RESTMetadata << RESTendl;

        RESTMetadata << "path for output plots: " << fModuleCanvasSave << RESTendl;

        EndPrintProcess();
    }

    const char* GetProcessName() const override { return "readoutAnalysis"; }

    // Constructor
    TRestRawReadoutAnalysisProcess();
    TRestRawReadoutAnalysisProcess(const char* configFilename);
    // Destructor
    ~TRestRawReadoutAnalysisProcess();

    ClassDefOverride(TRestRawReadoutAnalysisProcess, 1);  // Template for a REST "event process" class
                                                          // inherited from TRestEventProcess
};
#endif
