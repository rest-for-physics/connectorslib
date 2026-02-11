//*******************************************************************************************************
//*** Description: This macro receives as input two variable names that correspond to a file with the raw data
//*** and the file with the readout.
//*** It creates readout channel activity plots for X and Y directions and for AGET ID.
//***
//**********************************************************************************************************

int REST_Connectors_RawReadoutChannelActivityCorrelation(
    string fName,
    TString fReadout,
    std::string cut="",
    Int_t nEntries=0
) {
    //// Readout ////
    std::cout << "Opening readout file: " << fReadout << std::endl;
    TFile* f = new TFile(fReadout);
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
    std::cout << "Opening TRestRun file: " << fName << std::endl;
    TRestRun* run = new TRestRun(fName);
    double startTimeStamp = run->GetStartTimestamp();
    double endTimeStamp = run->GetEndTimestamp();

    std::cout << "Getting modules..." << std::endl;
    std::vector<TRestDetectorReadoutModule*> modules;
    for (size_t p=0; p<readout->GetNumberOfReadoutPlanes(); p++) {
        TRestDetectorReadoutPlane* plane = readout->GetReadoutPlane(p);
        for (size_t m=0; m<plane->GetNumberOfModules(); m++) {
            modules.push_back(plane->GetModule(m));
        }
    }

    std::cout << "Creating histograms..." << std::endl;
    std::vector<TH1D*> hChannelActivityID;
    std::vector<TH2D*> hChannelCorrelationX;
    std::vector<TH2D*> hChannelCorrelationY;
    std::vector<TH2D*> hChannelCorrelationXY;
    for (size_t m = 0; m < modules.size(); m++) {
        auto module = modules[m];
        hChannelActivityID.push_back(new TH1D(Form("hChActID_m%zu", m), Form("Readout ID - module %zu", m), module->GetMaxDaqID()-module->GetMinDaqID(), module->GetMinDaqID(), module->GetMaxDaqID()));
        hChannelCorrelationX.push_back(new TH2D(Form("hChCorrX_m%zu", m), Form("ChCorr X - module %zu", m), module->GetNumberOfChannels()/2, module->GetOrigin().X(),
                            module->GetOrigin().X() + module->GetSize().X(), module->GetNumberOfChannels()/2, module->GetOrigin().X(),
                            module->GetOrigin().X() + module->GetSize().X()));
        hChannelCorrelationY.push_back(new TH2D(Form("hChCorrY_m%zu", m), Form("ChCorr Y - module %zu", m), module->GetNumberOfChannels()/2, module->GetOrigin().Y(),
                            module->GetOrigin().Y() + module->GetSize().Y(), module->GetNumberOfChannels()/2, module->GetOrigin().Y(),
                            module->GetOrigin().Y() + module->GetSize().Y()));
        hChannelCorrelationXY.push_back(new TH2D(Form("hChCorrXY_m%zu", m), Form("ChCorr XY - module %zu", m), module->GetNumberOfChannels()/2, module->GetOrigin().X(),
                            module->GetOrigin().X() + module->GetSize().X(), module->GetNumberOfChannels()/2, module->GetOrigin().Y(),
                            module->GetOrigin().Y() + module->GetSize().Y()));
    }

    std::cout << "Looping over entries..." << std::endl;
    // Loop over entries and signals per entry
    TRestRawSignalEvent* fRawSignalEvent = new TRestRawSignalEvent();
    Int_t nEntriesToProcess = run->GetEntries();
    if (nEntries > 0 && nEntries < run->GetEntries())
        nEntriesToProcess = nEntries;
    
    TRestAnalysisTree* analysisTree = run->GetAnalysisTree();
    for (Int_t i = 0; i < nEntriesToProcess; i++) {
        run->GetEntry(i);
        auto progressPercentage= (i+1)*100./nEntriesToProcess;
        if (i==0 || i%1000==0 || i==nEntriesToProcess-1)
            std::cout << "\rEntry: " << i << " / " << nEntriesToProcess << " (" << progressPercentage << "%)" << std::flush;

        if (cut != "") {
            if (!analysisTree->EvaluateCuts(cut.c_str())) {
                continue;
            }
        }

        fRawSignalEvent = (TRestRawSignalEvent*)run->GetInputEvent();
        for (int k = 0; k < fRawSignalEvent->GetNumberOfSignals(); k++) {
            TRestRawSignal* sigA = fRawSignalEvent->GetSignal(k);
            auto signalID = sigA->GetID();
            // find the module corresponding to the signal ID
            for (size_t m=0; m<modules.size(); m++) {
                auto module = modules[m];
                if (!module->IsDaqIDInside(signalID)) {
                    continue;
                }
                hChannelActivityID[m]->Fill(signalID);
                
                double xA = readout->GetX(signalID);
                double yA = readout->GetY(signalID);
                if (!isnan(xA)) {
                    for (auto sID : fRawSignalEvent->GetSignalIds()) {
                        double xB = readout->GetX(sID);
                        if (!isnan(xB)) {
                            hChannelCorrelationX[m]->Fill(xA, xB);
                        } else {
                            double yB = readout->GetY(sID);
                            if (!isnan(yB)) {
                                hChannelCorrelationXY[m]->Fill(xA, yB);
                            }
                        }
                    }
                }
                if (!isnan(yA)) {
                    for (auto sID : fRawSignalEvent->GetSignalIds()) {
                        double yB = readout->GetY(sID);
                        if (!isnan(yB)) {
                            hChannelCorrelationY[m]->Fill(yA, yB);
                        }
                    }
                }
            }
        }
    }

    std::cout << "Generating and plotting histograms..." << std::endl;
    for (size_t m = 0; m < modules.size(); m++) {
        auto mod = modules[m];
        // Plot readout channel activity
        TCanvas* cID = new TCanvas(Form("cID_module%zu", m));
        hChannelActivityID[m]->SetFillColor(4);
        hChannelActivityID[m]->Draw("histo");
        hChannelActivityID[m]->GetXaxis()->SetTitle(Form("ID readout channel - Module %zu", m));
        
        // Generate TGraph of readout channel activity ordered by its position in X and Y
        TGraph* gX = new TGraph();
        TGraph* gY = new TGraph();
        gX->SetTitle(Form("Readout X - module %zu", m));
        gY->SetTitle(Form("Readout Y - module %zu", m));
        for (int bx = 1; bx <= hChannelActivityID[m]->GetNbinsX(); bx++) {
            int signalID = hChannelActivityID[m]->GetBinCenter(bx);
            auto x = readout->GetX(signalID);
            auto y = readout->GetY(signalID);
            auto counts = hChannelActivityID[m]->GetBinContent(bx);
            if (!isnan(x)) {
                gX->AddPoint(x, counts);
            }
            if (!isnan(y)) {
                gY->AddPoint(y, counts);
            }
        }
        gX->SetFillColor(38);
        gX->SetMarkerColor(4);
        gY->SetFillColor(38);
        gY->SetMarkerColor(4);
        //gX->SetMarkerSize(0.2);

        TCanvas* cCAXY = new TCanvas(Form("cCAXY_module%zu", m), Form("X Y - %zu", m));
        cCAXY->Divide(2,2);
        cCAXY->cd(1);
        gX->Draw("APB");
        gX->GetXaxis()->SetTitle(Form("X readout channel (mm) - Module %zu", m));
        

        cCAXY->cd(2);
        gY->Draw("APB");
        gY->GetXaxis()->SetTitle(Form("Y readout channel (mm) - Module %zu", m));

        // Plot channel correlation in X and Y
        cCAXY->cd(3);
        hChannelCorrelationX[m]->SetStats(0);
        hChannelCorrelationX[m]->Draw("histo colz");
        hChannelCorrelationX[m]->GetXaxis()->SetTitle(Form("X readout channel (mm) - Module %zu", m));
        hChannelCorrelationX[m]->GetYaxis()->SetTitle(Form("X readout channel (mm) - Module %zu", m));

        cCAXY->cd(4);
        hChannelCorrelationY[m]->SetStats(0);
        hChannelCorrelationY[m]->Draw("histo colz");
        hChannelCorrelationY[m]->GetXaxis()->SetTitle(Form("Y readout channel (mm) - Module %zu", m));
        hChannelCorrelationY[m]->GetYaxis()->SetTitle(Form("Y readout channel (mm) - Module %zu", m));

        TCanvas *cCCorrXY = new TCanvas(Form("cCCorrXY_module%zu", m), Form("X Y - %zu", m));
        hChannelCorrelationXY[m]->SetStats(0);
        hChannelCorrelationXY[m]->Draw("histo colz");
        hChannelCorrelationXY[m]->GetXaxis()->SetTitle(Form("X readout channel (mm) - Module %zu", m));
        hChannelCorrelationXY[m]->GetYaxis()->SetTitle(Form("Y readout channel (mm) - Module %zu", m));

    }

    return 0;
}

