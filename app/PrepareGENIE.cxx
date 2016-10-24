#include <stdio.h>
#include <stdlib.h>
#include "event1.h"
#include "params_all.h"
#include "params.h"
#include "TFile.h"
#include "TH1D.h"
#include "TTree.h"
#include "PlotUtils.h"

#ifdef __GENIE_ENABLED__
#include "Conventions/Units.h"
#endif

bool gFlagMerge = false;
std::string gInputFiles = "";
std::string gOutputFile = "";
std::string gFluxFile   = "";
std::string gTarget     = "";

void PrintOptions();
void ParseOptions(int argc, char* argv[]);
void RunGENIEMerger(std::string inputs, std::string output);
void RunGENIEPrepare(std::string input, std::string flux, std::string target, std::string output);

int main(int argc, char* argv[]){

  ParseOptions(argc, argv);

  if (gFlagMerge) RunGENIEMerger(gInputFiles, gOutputFile);
  else            RunGENIEPrepare(gInputFiles, gFluxFile, gTarget, gOutputFile);

}

void RunGENIEMerger(std::string inputs, std::string output){

};

void RunGENIEPrepare(std::string input, std::string flux, std::string target, std::string output){

  std::cout << "Running GENIE Prepare" << std::endl;

  // Setup TTree
  TChain* tn = new TChain("gtree");
  tn->AddFile(input.c_str());

  int nevt = tn->GetEntries();
  NtpMCEventRecord * genientpl = NULL;
  
  tn->SetBranchAddress("gmcrec",&genientpl);

  // Get Flux Hist
  std::vector<std::string> fluxvect = PlotUtils::ParseToStr(flux,",");
  TH1D* fluxhist = NULL;
  if (fluxvect.size() > 1){
    TFile* fluxfile = new TFile(fluxvect[0].c_str(),"READ");
    fluxhist = (TH1D*) fluxfile->Get(fluxvect[1].c_str());
    fluxhist->SetDirectory(0);
  } else {
    std::cout << "NO FLUX SPECIFIED" << std::endl;
    throw;
  }

  // Make Event Hist
  TH1D* eventhist = (TH1D*)fluxhist->Clone();
  eventhist->Reset();

  TH1D* xsechist = (TH1D*) eventhist->Clone();
  
  // Create maps
  std::map<std::string, TH1D*> modexsec;
  std::map<std::string, TH1D*> modecount;
  std::vector<std::string> genieids;
  std::vector<std::string> targetids;
  std::vector<std::string> interids;
  
  
  // Loop over all events
  for (int i = 0; i < nevt; i++) {
    tn->GetEntry(i);

    EventRecord& event = *(genientpl->event);
    GHepParticle* neu = event.Probe();

    // Get XSec From Spline
    GHepRecord genie_record = static_cast<GHepRecord>(event);
    double xsec = (genie_record.XSec() / (1E-38 * genie::units::cm2));

    // Parse Interaction String
    std::string mode = genie_record.Summary()->AsString();
    std::vector<std::string> modevec = PlotUtils::ParseToStr(mode,";");
    std::string targ  = ( modevec[0] + ";" + modevec[1] );
    std::string inter = mode;

    // Fill lists of Unique IDS
    if (std::find(targetids.begin(), targetids.end(),targ)
	== targetids.end()){
      targetids.push_back(targ);
    }
    
    if (std::find(interids.begin(), interids.end(), inter)
	== interids.end()){
      interids.push_back(inter);
    }

    // Create entries Mode Maps
    if (modexsec.find(mode) == modexsec.end()){
      genieids.push_back(mode);
      
      modexsec[mode] = (TH1D*)xsechist->Clone();
      modecount[mode] = (TH1D*)xsechist->Clone();
    }

    // Fill XSec Histograms
    modexsec[mode]->Fill(neu->E(), xsec);
    modecount[mode]->Fill(neu->E());

    // Fill total event hist
    eventhist->Fill(neu->E());

    // Clear Event
    genientpl->Clear();
  }
  std::cout << "Processed all events" << std::endl;

  // Once event loop is done we can start saving stuff into the file
  bool savesplines = FitPar::Config().GetParB("save_genie_splines");
  
  TFile* outputfile = new TFile(input.c_str(),"UPDATE");
  outputfile->cd();

  std::cout << "Getting splines " << std::endl;
  
  // Save each of the reconstructed splines to file
  std::map<std::string, TH1D*> modeavg;

  
  TDirectory* inddir = (TDirectory*) outputfile->Get("IndividualGENIESplines");
  if (!inddir) inddir = (TDirectory*)outputfile->mkdir("IndividualGENIESplines");
  inddir->cd();
  
  for (UInt_t i = 0; i < genieids.size(); i++){
    std::string mode = genieids[i];

    modexsec[mode]->Write(  (mode + "_summed_xsec").c_str() , TObject::kOverwrite);
    modecount[mode]->Write( (mode + "_summed_evt").c_str()  , TObject::kOverwrite);

    //Form extra avg xsec map -> Reconstructed spline
    modeavg[mode] = (TH1D*)modexsec[mode]->Clone();
    modeavg[mode]->Divide(modecount[mode]);

    modeavg[mode]->Write( (mode + "_rec_spline").c_str() , TObject::kOverwrite);
  }

  TDirectory* targdir = (TDirectory*) outputfile->Get("TargetGENIESplines");
  if (!targdir) targdir = (TDirectory*) outputfile->mkdir("TargetGENIESplines");
  targdir->cd();
  
  std::cout << "Getting Target Splines" << std::endl;
  // For each target save a total spline
  std::map<std::string, TH1D*> targetsplines;

  for (int i = 0; i < targetids.size(); i++){
    std::cout << "Getting target "<< i << std::endl;
    std::string targ = targetids[i];
    targetsplines[targ] = (TH1D*) xsechist->Clone();
    std::cout << "Created target spline for " << targ << std::endl;
    
    for (int j = 0; j < genieids.size(); j++){
      std::string mode = genieids[j];

      // Look at all matching modes/targets
      if (mode.find(targ) != std::string::npos){

	std::cout << "Mode " << mode << " contains " << targ << " target!" << std::endl;
	modeavg[mode]->Write( (mode + "_cont_" + targ).c_str() , TObject::kOverwrite);
	targetsplines[targ]->Add( modeavg[mode] );
	std::cout << "Finished with Mode " << mode << " "  << modeavg[mode]->Integral()<<std::endl;
      }
    }

    std::cout << "Saving target spline:" <<targ<< std::endl;
    targetsplines[targ]->Write(("Total" + targ).c_str(), TObject::kOverwrite);
  }

  std::cout << "Getting total splines" << std::endl;
  // Now we have each of the targets we need to create a total cross-section.
  int totalnucl = 0;
  std::vector<std::string> targprs = PlotUtils::ParseToStr(target,",");
  TH1D* totalxsec = (TH1D*) xsechist->Clone();
  
  for (int i = 0; i < targprs.size(); i++){
    std::string targpdg = targprs[i];

    for (std::map<std::string, TH1D*>::iterator iter = targetsplines.begin();
	 iter != targetsplines.end(); iter++){
      std::string targstr = iter->first;
      TH1D* xsec = iter->second;

      if (targstr.find(targpdg) != std::string::npos){
	std::cout << "Adding target spline " << targstr << " : " << xsec->Integral("width") << std::endl;
	totalxsec->Add(xsec);

	int nucl = atoi( targpdg.c_str() );
	totalnucl += int((nucl  % 10000) / 10);

      }
    }
  }

  outputfile->cd();
  totalxsec->Write("nuisance_xsec", TObject::kOverwrite);
  eventhist = (TH1D*)fluxhist->Clone();
  eventhist->Multiply(totalxsec);

  std::cout << "Dividing by Total Nucl = " << totalnucl << std::endl;
  eventhist->Scale(1.0 / double(totalnucl) );

  eventhist->Write("nuisance_events", TObject::kOverwrite);
  fluxhist->Write("nuisance_flux", TObject::kOverwrite);

  
  std::cout << "Inclusive XSec Per Nucleon = " << eventhist->Integral("width") * 1E-38 / fluxhist->Integral("width") << std::endl;

  
  


  

  return;
};



void PrintOptions(){

  std::cout << "PrepareGENIEEvents NUISANCE app. "  << std::endl
	    << "Takes GHep Outputs and prepares events for NUISANCE." << std::endl << std::endl
	    << "PrepareGENIEEvents  [-h,-help,--h,--help]  [-m] [-i inputfile1.root,inputfile2.root,inputfile3.root,...] " 
	    << "[-o outputfile.root] [-f flux_root_file.root,flux_hist_name] [-t target1[frac1],target2[frac2],...]" 
	    << std::endl << std::endl;

  std::cout << "Prepare Mode [Default] : Takes a single GHep file, reconstructs the original GENIE splines, " 
	    << " and creates a duplicate file that also contains the flux, event rate, and xsec predictions that NUISANCE needs. " << std::endl;
  std::cout << "Following options are required for Prepare Mode:" << std::endl;
  std::cout << " [ -i inputfile.root  ] : Reads in a single GHep input file that needs the xsec calculation ran on it. " <<std::endl;
  std::cout << " [ -o outputfile.root ] : Destination of output file" << std::endl;
  std::cout << " [ -f flux_file.root,hist_name ] : Path to root file containing the flux histogram the GHep records were generated with." 
	    << " A simple method is to point this to the flux histogram genie generatrs '-f /path/to/events/input-flux.root,spectrum'. " << std::endl;
  std::cout << " [ -t target ] : Target that GHepRecords were generated with. Should match the string used in gevgen. " << std::endl;

  std::cout << "Merger Mode [activate with -m] : Takes the list of input files assuming 'Prepare Mode' has already been ran on them and merges them " 
	    << "into a single file with associated Friend Tree to help with conserving ratios of events (e.g. adding nue and nueb beams together into a single file" << std::endl;
  std::cout << "Following optoins are required for Merger Mode:" << std::endl;
  std::cout << " [ -i inputfile1.root,inputfile2.root ] : Comma Seperated list of files to be merged. " << std::endl;
  std::cout << " [ -o outputfile.root ] : Output file for merger." << std::endl;

}

void ParseOptions(int argc, char* argv[]){
  bool flagopt = false;

  // If No Arguments print commands                                                                                                                                                                                                        
  for (int i = 1; i< argc; ++i){
    if (!std::strcmp(argv[i], "-h"))   { flagopt  = true; break; }
    if (!std::strcmp(argv[i], "-m"))   { gFlagMerge = true; break; }
    if (i+1 != argc){

      // Cardfile                                                                                                                                                                                                                          
      if (!std::strcmp(argv[i], "-h"))      { flagopt = true; break; }
      else if (!std::strcmp(argv[i], "-i")) { gInputFiles = argv[i+1]; ++i; }
      else if (!std::strcmp(argv[i], "-o")) { gOutputFile = argv[i+1]; ++i; }
      else if (!std::strcmp(argv[i], "-f")) { gFluxFile   = argv[i+1]; ++i; }
      else if (!std::strcmp(argv[i], "-t")) { gTarget     = argv[i+1]; ++i; } 
      else {
	std::cerr << "ERROR: unknown command line option given! - '"
                  <<argv[i]<<" "<<argv[i+1]<<"'"<< std::endl;
	PrintOptions();
	break;
      }
    }
  }

  if (gOutputFile == "" && !flagopt){ 
    std::cerr << "No output file specificed!" << std::endl; 
    flagopt = true;
  }

  if (gInputFiles == "" && !flagopt){
    std::cerr << "No input file(s) specified!" << std::endl;
    flagopt = true;
  }
    
  if (!gFlagMerge && gFluxFile == "" && !flagopt){
    std::cerr << "No flux input specified for Prepare Mode" << std::endl;
    flagopt = true;
  }

  if (!gFlagMerge && gTarget == "" && !flagopt){
    std::cerr << "No target specified for Prepare Mode" << std::endl;
    flagopt = true;
  }

  if (argc < 1 || flagopt){
     PrintOptions();
     exit(-1);
  }

  return;
}