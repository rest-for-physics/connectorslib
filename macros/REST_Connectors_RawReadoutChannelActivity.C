//*******************************************************************************************************
//*** Description: This macro receives as input two variable names that correspond to a file with the raw data
//*** and the file with the readout.
//*** It creates readout channel activity plots for X and Y directions and for AGET ID.
//***
//**********************************************************************************************************

Int_t REST_Connectors_RawReadoutChannelActivity(std::string fName, TString fReadout, std::string cut = "",
                                                Int_t nEntries = 0) {
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
    for (size_t p = 0; p < readout->GetNumberOfReadoutPlanes(); p++) {
        TRestDetectorReadoutPlane* plane = readout->GetReadoutPlane(p);
        for (size_t m = 0; m < plane->GetNumberOfModules(); m++) {
            modules.push_back(plane->GetModule(m));
        }
    }

    std::cout << "Creating histograms..." << std::endl;
    std::vector<TH1D*> hChannelActivityID;
    std::vector<TH2D*> hChannelActivityIDTime;
    for (size_t m = 0; m < modules.size(); m++) {
        auto module = modules[m];
        hChannelActivityID.push_back(new TH1D("", Form("Readout ID - module %zu", m),
                                              module->GetMaxDaqID() - module->GetMinDaqID(),
                                              module->GetMinDaqID(), module->GetMaxDaqID()));
        hChannelActivityIDTime.push_back(new TH2D(
            "", Form("ChAct ID - Time - module %zu", m), 100, startTimeStamp, endTimeStamp,
            module->GetMaxDaqID() - module->GetMinDaqID(), module->GetMinDaqID(), module->GetMaxDaqID()));
    }

    std::cout << "Looping over entries..." << std::endl;
    // Loop over entries and signals per entry
    TRestRawSignalEvent* fRawSignalEvent = new TRestRawSignalEvent();
    Int_t nEntriesToProcess = run->GetEntries();
    if (nEntries > 0 && nEntries < run->GetEntries()) nEntriesToProcess = nEntries;

    TRestAnalysisTree* analysisTree = run->GetAnalysisTree();
    for (Int_t i = 0; i < nEntriesToProcess; i++) {
        run->GetEntry(i);
        auto progressPercentage = (i + 1) * 100. / nEntriesToProcess;
        if (i == 0 || i % 1000 == 0 || i == nEntriesToProcess - 1)
            std::cout << "\rEntry: " << i << " / " << nEntriesToProcess << " (" << progressPercentage << "%)"
                      << std::flush;

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
            for (size_t m = 0; m < modules.size(); m++) {
                auto module = modules[m];
                if (module->IsDaqIDInside(signalID)) {
                    hChannelActivityID[m]->Fill(signalID);
                    hChannelActivityIDTime[m]->Fill(fRawSignalEvent->GetTimeStamp(), signalID);
                }
            }
        }
    }

    std::cout << "Generating and plotting histograms..." << std::endl;
    for (size_t m = 0; m < modules.size(); m++) {
        auto mod = modules[m];
        // Plot readout channel activity
        TCanvas* cChAct = new TCanvas(Form("cChAct_m%zu", m), Form("Module %zu - Channel Activity", m), 1200, 400);
        cChAct->Divide(3, 1);

        cChAct->cd(1);
        hChannelActivityID[m]->SetStats(0);
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

        cChAct->cd(2);
        gX->SetFillColor(38);
        gX->SetMarkerColor(4);
        gX->Draw("APB");
        gX->GetXaxis()->SetTitle(Form("X readout channel (mm) - Module %zu", m));

        cChAct->cd(3);
        gY->SetFillColor(38);
        gY->SetMarkerColor(4);
        gY->Draw("APB");
        gY->GetXaxis()->SetTitle(Form("Y readout channel (mm) - Module %zu", m));

        // Generate 2D histogram of readout channel activity in time ordered by its position in X and Y
        TH2D* hChannelActivityXTime = new TH2D(
            "", Form("ChAct X - Time - module %zu", m), 100, startTimeStamp, endTimeStamp,
            mod->GetNumberOfChannels() / 2, mod->GetOrigin().X(), mod->GetOrigin().X() + mod->GetSize().X());
        TH2D* hChannelActivityYTime = new TH2D(
            "", Form("ChAct Y - Time - module %zu", m), 100, startTimeStamp, endTimeStamp,
            mod->GetNumberOfChannels() / 2, mod->GetOrigin().Y(), mod->GetOrigin().Y() + mod->GetSize().Y());
        for (int bx = 1; bx <= hChannelActivityIDTime[m]->GetNbinsY(); bx++) {
            int signalID = hChannelActivityIDTime[m]->GetYaxis()->GetBinCenter(bx);
            auto x = readout->GetX(signalID);
            auto y = readout->GetY(signalID);
            for (int tx = 1; tx <= hChannelActivityIDTime[m]->GetNbinsX(); tx++) {
                double time = hChannelActivityIDTime[m]->GetXaxis()->GetBinCenter(tx);
                double counts = hChannelActivityIDTime[m]->GetBinContent(tx, bx);
                if (!isnan(x) && counts > 0) {
                    hChannelActivityXTime->Fill(time, x, counts);
                }
                if (!isnan(y) && counts > 0) {
                    hChannelActivityYTime->Fill(time, y, counts);
                }
            }
        }

        TCanvas* cChActTime = new TCanvas(Form("cChActTime_m%zu", m), Form("Module %zu - Channel Activity in Time", m));
        cChActTime->Divide(2, 1);

        cChActTime->cd(1);
        hChannelActivityXTime->SetStats(0);
        hChannelActivityXTime->Draw("histo colz");
        hChannelActivityXTime->GetYaxis()->SetTitle(Form("X readout channel (mm) - Module %zu", m));
        hChannelActivityXTime->GetXaxis()->SetTitle("Time");
        hChannelActivityXTime->GetXaxis()->SetTimeDisplay(1);

        cChActTime->cd(2);
        hChannelActivityYTime->SetStats(0);
        hChannelActivityYTime->Draw("histo colz");
        hChannelActivityYTime->GetYaxis()->SetTitle(Form("Y readout channel (mm) - Module %zu", m));
        hChannelActivityYTime->GetXaxis()->SetTitle("Time");
        hChannelActivityYTime->GetXaxis()->SetTimeDisplay(1);
    }

    return 0;
}
