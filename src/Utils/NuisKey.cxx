#include "NuisKey.h"

std::string nuiskey::GetS(std::string name){ 
  return Config::Get().GetS(fNode,name); 
};

int         nuiskey::GetI(std::string name){ 
  return Config::Get().GetI(fNode,name); 
};

double      nuiskey::GetD(std::string name){ 
  return Config::Get().GetD(fNode,name); 
};

bool        nuiskey::GetB(std::string name){ 
  return Config::Get().GetB(fNode,name); 
};

std::vector<std::string> nuiskey::GetVS(std::string name, const char* del){
  return Config::Get().GetVS(fNode,name,del);
};

std::vector<int>         nuiskey::GetVI(std::string name, const char* del){
  return Config::Get().GetVI(fNode,name,del);
};

std::vector<double>      nuiskey::GetVD(std::string name, const char* del){
  return Config::Get().GetVD(fNode,name,del);
};


std::vector<nuiskey> Config::QueryKeys(const std::string type, const std::string test1){

  // Get Vector of nodes     
  std::vector<XMLNodePointer_t> nodelist = Config::Get().GetNodes(type);

  // Convert List into a key list for easier access  
  std::vector<nuiskey> keylist;
  for (std::vector<XMLNodePointer_t>::const_iterator iter = nodelist.begin();
       iter != nodelist.end(); iter++){

    // Create new key
    nuiskey newkey = nuiskey(*iter);

    // Add test1
    if (!test1.empty()){
      std::vector<std::string> testvect = GeneralUtils::ParseToStr(test1,"=");
      if (testvect.size() < 2) continue;
      if (newkey.GetS(testvect[0]) != testvect[1]) continue;
    }

    // Save node as nuiskey  
    keylist.push_back( newkey );
  }

  // Return list of keys     
  return keylist;
}

nuiskey Config::QueryLastKey(const std::string type, const std::string test1){
  // Loop over all for now because I'm lazy...
  std::vector<nuiskey> allkeys = Config::QueryKeys(type,test1);
  if (allkeys.size() < 1) return nuiskey();
  else return allkeys[allkeys.size()-1];
}


nuiskey Config::QueryFirstKey(const std::string type, const std::string test1){
  // Loop over all for now because I'm lazy...          
  std::vector<nuiskey> allkeys = Config::QueryKeys(type,test1);
  if (allkeys.size() < 1) return nuiskey();
  else return allkeys[allkeys.size()-1];
}

nuiskey Config::CreateParameterKeyFromLine(const std::string line){
  nuiskey parameterkey = Config::CreateKey("parameter");

  // Parse
  std::vector<std::string> strvct = GeneralUtils::ParseToStr(line, " ");

  // Add to key
  parameterkey.AddS("name", strvct[1]);
  parameterkey.AddS("nominal", strvct[2]);

  if (strvct.size() == 7){
    parameterkey.AddS("low",strvct[3]);
    parameterkey.AddS("high",strvct[4]);
    parameterkey.AddS("step",strvct[5]);
    parameterkey.AddS("state",strvct[6]);
  } else if (strvct.size() == 3){
    parameterkey.AddS("state","FIX");
  }

  return parameterkey;
}

bool nuiskey::Has(const std::string name){
  return Config::Get().Has(fNode, name);
}

nuiskey Config::CreatePullKeyFromLine(const std::string line){
  nuiskey pullkey = Config::CreateKey("pull");
  return pullkey;
}


nuiskey Config::CreateOldConfigKeyFromLine(const std::string line){
nuiskey configkey = Config::CreateKey("config");
return configkey;
}

nuiskey Config::CreateSampleKeyFromLine(const std::string line){

  // Create new key
  nuiskey samplekey = Config::CreateKey("sample");

  // Parse
  std::vector<std::string> strvct = GeneralUtils::ParseToStr(line, " ");

  // Get elements
  samplekey.AddS("name"  , strvct[1]);
  samplekey.AddS("input" , strvct[2]);
  if (strvct.size() > 3){
    samplekey.AddS("type", strvct[3]);
  }
  if (strvct.size() > 4){
    samplekey.AddS("norm", strvct[4]);
  }

  return samplekey;
}


nuiskey Config::CreateKey(const std::string name){
  return nuiskey(Config::Get().CreateNode(name));
}

void nuiskey::AddS(std::string name, std::string newval){
  Config::Get().AddS(fNode, name, newval);
}
void nuiskey::AddI(std::string name, int newval){
  Config::Get().AddI(fNode, name, newval);
}
void nuiskey::AddD(std::string name, double newval){
  Config::Get().AddD(fNode, name, newval);
}
void nuiskey::AddB(std::string name, bool newval){
  Config::Get().AddB(fNode, name, newval);
}



void Config::ConvertAndLoadCardToXMLFormat(const std::string cardfile){

  // Build XML Config from the card file
  std::vector<std::string> cardlines =
      GeneralUtils::ParseFileToStr(cardfile, "\n");
  int linecount = 0;

  for (std::vector<std::string>::iterator iter = cardlines.begin();
       iter != cardlines.end(); iter++) {
    std::string line = (*iter);
    linecount++;

    // Skip Comments
    if (line.empty()) continue;
    if (line.c_str()[0] == '#') continue;

    // Parse whitespace
    std::vector<std::string> strvct = GeneralUtils::ParseToStr(line, " ");
    if (strvct.empty()) continue;

    // Get Identifier
    std::string id = strvct[0];

    // Build backwards compatible xml configs
    if (!id.compare("sample")) Config::CreateSampleKeyFromLine(line);

    if (id.find("_parameter") != std::string::npos) 
      Config::CreateParameterKeyFromLine(line);

    if (!id.compare("covar") || 
        !id.compare("pull")  ||
        !id.compare("throw")) 
      Config::CreatePullKeyFromLine(line);

    if (!id.compare("config"))
      Config::CreateOldConfigKeyFromLine(line);
  }


  return;
}
