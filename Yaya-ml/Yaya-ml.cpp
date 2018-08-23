// Yaya-ml.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "YamlReader.h"
#include "StdStringFcn.h"

int _tmain(int argc, _TCHAR* argv[])
{
	char buffer[512];
	std::string path(argv[0]);
	std::string ExeDirectory = path.substr(0, path.find_last_of('\\') + 1);

	YamlReader yaml;
	std::string filename = ExeDirectory+ "\\Agent.cfg";
	yaml.LoadFromFile(filename);
	std::string HttpPort = yaml.Find("ROOT.Port");
	std::string ServiceName = yaml.Find("ROOT.ServiceName");
	_snprintf(buffer, sizeof(buffer), "Existing Http Port=%s ServiceName=%s\n" , HttpPort.c_str() , ServiceName.c_str()  );
	OutputDebugString(buffer);
	yaml.SetKeyValue("ROOT.ServiceName", "Razzmatazz");
	yaml.AddSection("ROOT.Adapters.Globex");
	yaml.SetKeyValue("ROOT.Adapters.Acme.Host", "127.0.0.2");
	yaml.SetKeyValue("ROOT.Adapters.Globex.Host", "127.0.0.2");
	yaml.SetKeyValue("ROOT.Adapters.Acme.Port", "7879");

	// This generates a string of the current 
	std::string str = yaml.ToString();
	OutputDebugString(str.c_str());

	return 0;
}



struct ptree
{
	std::string data;                                      // data associated with the node
	std::list<std::pair<std::string, ptree> >
		children;                                  // ordered list of named children
};
