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

#include "BNL_CC1pi0_XSec_1DEnu_nu.h"

// The constructor
BNL_CC1pi0_XSec_1DEnu_nu::BNL_CC1pi0_XSec_1DEnu_nu(std::string inputfile, FitWeight *rw, std::string type, std::string fakeDataFile){

  fName = "BNL_CC1pi0_XSec_1DEnu_nu";
  fPlotTitles = "; E_{#nu} (GeV); #sigma(E_{#nu}) (cm^{2}/neutron)";
  EnuMin = 0.;
  EnuMax = 6.0;
  fIsDiag = true;
  fNormError = 0.15;
  Measurement1D::SetupMeasurement(inputfile, type, rw, fakeDataFile);
  // THIS IS DEFINITELY DODGY WITH THE "CORRECTION"; EnuMax for BNL definitely stops at 3GeV because of flux uncertainties, although correction goes to 6!

  this->SetDataValues(GeneralUtils::GetTopLevelDir()+"/data/BNL/CC1pi0_on_n/BNL_CC1pi0_on_n_1986.txt");
  this->SetupDefaultHist();

  fFullCovar = StatUtils::MakeDiagonalCovarMatrix(fDataHist);
  covar = StatUtils::GetInvert(fFullCovar);

  this->fScaleFactor = (this->fEventHist->Integral("width")*1E-38)/(fNEvents+0.)*16./8.;
};


void BNL_CC1pi0_XSec_1DEnu_nu::FillEventVariables(FitEvent *event) {

    TLorentzVector Pnu = (event->PartInfo(0))->fP;
    TLorentzVector Pp;
    TLorentzVector Pmu;
    TLorentzVector Ppi0;

    // Loop over the particle stack
    for (UInt_t j = 2; j < event->Npart(); ++j){
      if (!(event->PartInfo(j))->fIsAlive || (event->PartInfo(j))->fNEUTStatusCode != 0) continue;
      int PID = (event->PartInfo(j))->fPID;
      if (PID == 111) {
        Ppi0 = event->PartInfo(j)->fP;
      } else if (PID == 13) {
        Pmu = (event->PartInfo(j))->fP;
      } else if (PID == 2212) {
        Pp = event->PartInfo(j)->fP;
      }
    }

    //double hadMass = FitUtils::MpPi(Pp, Ppi0);

    //BNL doesn't have a W cut for CC1pi0 sadly (I'm super happy if you can find it!)
    double Enu = FitUtils::EnuCC1pi0rec(Pnu, Pmu, Ppi0);

    fXVar = Enu;

    return;
  }


bool BNL_CC1pi0_XSec_1DEnu_nu::isSignal(FitEvent *event) {
  return SignalDef::isCC1pi3Prong(event, 14, 111, 2112,EnuMin,EnuMax);
}


/*
void BNL_CC1pi0_XSec_1DEnu_nu::FillHistograms() {

  if (makeHadronicMassHist) {
    hadMassHist->Fill(hadMass);
  }

  Measurement1D::FillHistograms();

}


void BNL_CC1pi0_XSec_1DEnu_nu::ScaleEvents() {

  PlotUtils::FluxUnfoldedScaling(fMCHist, fFluxHist);
  PlotUtils::FluxUnfoldedScaling(fMCFine, fFluxHist);

  fMCHist->Scale(fScaleFactor);
  fMCFine->Scale(fScaleFactor);

  return;
}
*/
