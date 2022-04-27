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

//#include <TCanvas.h>

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

    void InitFromConfigFile();

    void Initialize();

    std::string fModuleCanvasSave;  //!

    // plots (saved directly in root file)
    std::map<int, TH2D*> fModuleHitMaps;    //! [MM id, channel activity]
    std::map<int, TH1D*> fModuleActivityX;  //! [MM id, channel activity]
    std::map<int, TH1D*> fModuleActivityY;  //! [MM id, channel activity]
    std::map<int, TH2D*> fModuleBSLSigmaX;  //! [MM id, channel activity]
    std::map<int, TH2D*> fModuleBSLSigmaY;  //! [MM id, channel activity]
                                            //

   public:
    any GetInputEvent() const override { return fSignalEvent; }
    any GetOutputEvent() const override { return fSignalEvent; }

    void InitProcess();
    TRestEvent* ProcessEvent(TRestEvent* inputEvent);
    void EndProcess();

    void PrintMetadata() {
        BeginPrintProcess();

        metadata << "channel activity and hitmap histograms required for module: ";
        auto iter2 = fModuleHitMaps.begin();
        while (iter2 != fModuleHitMaps.end()) {
            metadata << iter2->first << ", ";
            iter2++;
        }
        metadata << endl;

        metadata << "path for output plots: " << fModuleCanvasSave << endl;

        EndPrintProcess();
    }

    const char* GetProcessName() const override { return "readoutAnalysis"; }

    // Constructor
    TRestRawReadoutAnalysisProcess();
    TRestRawReadoutAnalysisProcess(const char* configFilename);
    // Destructor
    ~TRestRawReadoutAnalysisProcess();

    ClassDef(TRestRawReadoutAnalysisProcess, 1);  // Template for a REST "event process" class inherited from
                                                  // TRestEventProcess
};
#endif
