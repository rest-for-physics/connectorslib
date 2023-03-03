#include <cstdio>
#include <cstdlib>
#include "TFile.h"

Int_t REST_Connectors_RawReadoutChannelActivity(string fName, TString fReadout) {
    //// Readout ////
    TFile* f =
        new TFile(fReadout);  //"/media/storage/home/davidp/git/trexdm-readouts/readoutsMM4S_2MM_v5.root"
    TRestDetectorReadout* readout = NULL;

    // Search KEY of TRestDetectorReadout class.
    TIter nextkey(f->GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)nextkey()) != NULL) {
        if (key->GetClassName() == (TString) "TRestDetectorReadout") {
            if (readout == NULL) readout = (TRestDetectorReadout*)f->Get(key->GetName());
        }
    }
    delete key;
    readout->PrintMetadata(2);

    // Histogram size form readout dimensions
    TH1D* hChannelActivityX = new TH1D("", "Readout X", readout->GetNumberOfChannels() / 2, 0,
                                       readout->GetReadoutModuleWithID(0)->GetModuleSizeX());
    TH1D* hChannelActivityY = new TH1D("", "Readout Y", readout->GetNumberOfChannels() / 2, 0,
                                       readout->GetReadoutModuleWithID(0)->GetModuleSizeY());

    TRestRun* run = new TRestRun(fName);
    TRestRawSignalEvent* fRawSignalEvent = new TRestRawSignalEvent();

    // Loop over entries and signals per entry
    for (Int_t i = 0; i < run->GetEntries(); i++) {
        run->GetEntry(i);
        fRawSignalEvent = (TRestRawSignalEvent*)run->GetInputEvent();

        for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
            TRestRawSignal* sigA = fRawSignalEvent->GetSignal(k);
            double xA = readout->GetX(sigA->GetID());
            double yA = readout->GetY(sigA->GetID());

            if (!isnan(xA)) {
                hChannelActivityX->Fill(xA);
            }
            if (!isnan(yA)) {
                hChannelActivityY->Fill(yA);
            }
        }
    }
    delete run;
    delete fRawSignalEvent;

    // Plot readout channel activity
    TCanvas* cX1 = new TCanvas();
    hChannelActivityX->Draw("histo");
    hChannelActivityX->GetXaxis()->SetTitle("X readout channel");

    TCanvas* cY1 = new TCanvas();
    hChannelActivityY->Draw("histo");
    hChannelActivityY->GetXaxis()->SetTitle("Y readout channel");

    return 0;
}
