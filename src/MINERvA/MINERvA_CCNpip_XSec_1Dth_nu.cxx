// Copyright 2016 L. Pickering, P Stowell, R. Terri, C. Wilkinson, C. Wret

/*******************************************************************************
*    This file is part of NUISANCE.
*
*    NUISANCE is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    NUISANCE is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with NUISANCE.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#include "MINERvA_SignalDef.h"
#include "MINERvA_CCNpip_XSec_1Dth_nu.h"

//********************************************************************
// The constructor
MINERvA_CCNpip_XSec_1Dth_nu::MINERvA_CCNpip_XSec_1Dth_nu(std::string name, std::string inputfile, FitWeight *rw, std::string  type, std::string fakeDataFile){
//********************************************************************

  fName = name;
  fPlotTitles = "; #theta_{#pi} (degrees); (1/T#Phi) dN_{#pi}/d#theta_{#pi} (cm^{2}/degrees/nucleon)";

  // If we have full phase space we won't find 20deg in name
  fFullPhaseSpace = (fName.find("_20deg") == std::string::npos);
  // If we have updated data we won't have 2015 in name
  fUpdatedData = fName.find("2015") == std::string::npos;

  EnuMin = 1.5;
  EnuMax = 10;
  fIsDiag = false;
  Measurement1D::SetupMeasurement(inputfile, type, rw, fakeDataFile);

  // Reserve length 3 for the number of pions
  // We fill once per pion found in the event, so can fill multiple times for one event
  thVect.reserve(3);

  // So here is all the data we want to read in, pfoaw!
  // Same W cut for all releases

  // Full Phase Space
  if (fFullPhaseSpace) {

    // 2016 release data
    if (fUpdatedData) {

      this->SetDataValues(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2016/nu-ccNpi+-xsec-pion-angle.csv");

      // MINERvA has the error quoted as a percentage of the cross-section
      // Need to make this into an absolute error before we go from correlation matrix -> covariance matrix since it depends on the error in the ith bin
      for (int i = 0; i < fDataHist->GetNbinsX()+1; i++) {
        fDataHist->SetBinError(i+1, fDataHist->GetBinContent(i+1)*(fDataHist->GetBinError(i+1)/100.));
      }

      // This is a correlation matrix! but it's all fixed in SetCovarMatrixFromText
      this->SetCovarMatrixFromCorrText(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2016/nu-ccNpi+-correlation-pion-angle.csv", fDataHist->GetNbinsX());
    // 2015 release data
    } else {
      // 2015 release allows for shape comparisons

      if (fIsShape) {
        fName += "_shape";
        this->SetDataValues(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th_shape.txt");
        this->SetCovarMatrixFromCorrText(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th_shape_cov.txt", fDataHist->GetNbinsX());
      } else {
        this->SetDataValues(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th.txt");
        this->SetCovarMatrixFromCorrText(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th_cov.txt", fDataHist->GetNbinsX());
      }

      // Adjust MINERvA data to flux correction; roughly a 11% normalisation increase in data
      // Please change when MINERvA releases new data!
      for (int i = 0; i < fDataHist->GetNbinsX() + 1; i++) {
        fDataHist->SetBinContent(i+1, fDataHist->GetBinContent(i+1)*1.11);
      }
    }

  // Restricted Phase Space Data
  } else {

    // 2016 release data unfortunately not released in 20degree forward-going, revert to 2015 data
    if (fUpdatedData){
      LOG(SAM) << fName << " has no updated 2016 data for restricted phase space! Using 2015 data." << std::endl; 
      fUpdatedData = false;
    } 

    // Only 2015 20deg data
    if (fIsShape) {
      this->SetDataValues(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th_20deg_shape.txt");
      this->SetCovarMatrixFromCorrText(GeneralUtils::GetTopLevelDir()+
        "/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th_20deg_shape_cov.txt", fDataHist->GetNbinsX());
    } else {
        this->SetDataValues(GeneralUtils::GetTopLevelDir()+"/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th_20deg.txt");
        this->SetCovarMatrixFromCorrText(GeneralUtils::GetTopLevelDir()+
            "/data/MINERvA/CCNpip/2015/MINERvA_CCNpi_th_20deg_cov.txt", fDataHist->GetNbinsX());
    }
  }

  this->SetupDefaultHist();

  // Make some auxillary helper plots
  onePions  = (TH1D*)(fDataHist->Clone());
  twoPions  = (TH1D*)(fDataHist->Clone());
  threePions = (TH1D*)(fDataHist->Clone());
  morePions = (TH1D*)(fDataHist->Clone());

  onePions->SetNameTitle((fName+"_1pions").c_str(), (fName+"_1pions"+fPlotTitles).c_str());
  twoPions->SetNameTitle((fName+"_2pions").c_str(), (fName+"_2pions;"+fPlotTitles).c_str());
  threePions->SetNameTitle((fName+"_3pions").c_str(), (fName+"_3pions"+fPlotTitles).c_str());
  morePions->SetNameTitle((fName+"_4pions").c_str(), (fName+"_4pions"+fPlotTitles).c_str());


  fScaleFactor = this->fEventHist->Integral("width")*double(1E-38)/double(fNEvents)/TotalIntegratedFlux("width");
};


// ********************************************
// Fill the event variables
// Here we want to fill the angle for every pion we can find in the event
void MINERvA_CCNpip_XSec_1Dth_nu::FillEventVariables(FitEvent *event) {
// ********************************************

  thVect.clear();

  if (event->NumFSParticle(211) == 0 && event->NumFSParticle(-211) == 0) return;
  TLorentzVector Pnu = event->GetNeutrinoIn()->fP;
  TLorentzVector Ppip;

  // Loop over the particle stack
  for (unsigned int j = 2; j < event->Npart(); ++j) {

    // Only include alive particles
    if (!(event->PartInfo(j))->fIsAlive && (event->PartInfo(j))->fNEUTStatusCode != 0) continue;

    int PID = (event->PartInfo(j))->fPID;
    // Select highest momentum (energy) charged pion
    if (abs(PID) == 211) {
      Ppip = (event->PartInfo(j))->fP;
      double th = (180./M_PI)*FitUtils::th(Pnu, Ppip);
      thVect.push_back(th);
    }
  }

  fXVar = 0;

  return;
};

//********************************************************************
// The signal definition for MINERvA CCNpi+
// Last bool refers to if we're selecting on the full phase space or not
bool MINERvA_CCNpip_XSec_1Dth_nu::isSignal(FitEvent *event) {
//********************************************************************
  return SignalDef::isCCNpip_MINERvA(event, EnuMin, EnuMax, !fFullPhaseSpace);
}


//********************************************************************
// Need to override FillHistograms() here because we fill the histogram N_pion times
void MINERvA_CCNpip_XSec_1Dth_nu::FillHistograms() {
//********************************************************************

  if (Signal){

    int nPions = thVect.size();

    // Need to loop over all the pions in the sample
    for (size_t k = 0; k < thVect.size(); ++k) {

      double th = thVect[k];
      this->fMCHist->Fill(th, Weight);
      this->fMCFine->Fill(th, Weight);
      this->fMCStat->Fill(th, 1.0);

      if (nPions == 1) {
        onePions->Fill(th, Weight);
      } else if (nPions == 2) {
        twoPions->Fill(th, Weight);
      } else if (nPions == 3) {
        threePions->Fill(th, Weight);
      } else if (nPions > 3) {
        morePions->Fill(th, Weight);
      }

      PlotUtils::FillNeutModeArray(fMCHist_PDG, Mode, th, Weight);
    }
  }

  return;
}

//********************************************************************
void MINERvA_CCNpip_XSec_1Dth_nu::ScaleEvents() {
//********************************************************************
  Measurement1D::ScaleEvents();

  onePions->Scale(this->fScaleFactor, "width");
  twoPions->Scale(this->fScaleFactor, "width");
  threePions->Scale(this->fScaleFactor, "width");
  morePions->Scale(this->fScaleFactor, "width");

  return;
}

//********************************************************************
void MINERvA_CCNpip_XSec_1Dth_nu::Write(std::string drawOpts) {
//********************************************************************
  Measurement1D::Write(drawOpts);

  // Draw the npions stack
  onePions->SetTitle("1#pi");
  onePions->SetLineColor(kBlack);
  //onePions->SetFillStyle(0);
  onePions->SetFillColor(onePions->GetLineColor());

  twoPions->SetTitle("2#pi");
  twoPions->SetLineColor(kRed);
  //twoPions->SetFillStyle(0);
  twoPions->SetFillColor(twoPions->GetLineColor());

  threePions->SetTitle("3#pi");
  threePions->SetLineColor(kGreen);
  //threePions->SetFillStyle(0);
  threePions->SetFillColor(threePions->GetLineColor());

  morePions->SetTitle(">3#pi");
  morePions->SetLineColor(kBlue);
  //morePions->SetFillStyle(0);
  morePions->SetFillColor(morePions->GetLineColor());

  THStack pionStack = THStack((fName+"_pionStack").c_str(), (fName+"_pionStack").c_str());

  pionStack.Add(onePions);
  pionStack.Add(twoPions);
  pionStack.Add(threePions);
  pionStack.Add(morePions);

  pionStack.Write();

  return;
}
