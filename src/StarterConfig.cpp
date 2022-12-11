#include "StarterConfig.h"





template<typename T>
void tryAssing(boost::json::value const& json, std::string ptr, T& dest)
{
	try {
		std::error_code err;
		auto value = json.find_pointer(ptr, err);
		if (err) return;
		dest = boost::json::value_to<T>(*value);
	}
	catch (...) {
		// ignore exception
		// just end parsing 
	}

};




Starter_config tag_invoke(boost::json::value_to_tag< Starter_config >, boost::json::value const& json)
{
	Starter_config config;

	// copy parameters from json file
	tryAssing(json, "/TCPServer/port", config.server.port);
	tryAssing(json, "/App/autoStart", config.app.autoStart);
	tryAssing(json, "/App/projRoot", config.app.projRoot);
	tryAssing(json, "/App/exec", config.app.executable);
	tryAssing(json, "/Compilation/libraryPath", config.compilation.library_path);
	tryAssing(json, "/Compilation/includeDirectoryPath", config.compilation.include_directory);

	return config;
}




Starter_config read_config(std::filesystem::path path) {
	using namespace std;

	fstream configFile(path, ios_base::in | ios_base::binary);


	if (!configFile.is_open()) {
		cout << "Error: Cannot open ./starter.config file \n";
		return Starter_config();
	}

	string configStr;

	// ******* this code is from https://cplusplus.com/reference/istream/istream/read/
	{
		// get length of file
		configFile.seekg(0, configFile.end);
		size_t configFileLen = configFile.tellg();

		// set pos to start
		configFile.seekg(0, configFile.beg);

		// read from file and store to string 
		char* data = new char[configFileLen];
		configFile.read(data, configFileLen);
		configStr.assign(data, configFileLen);

		delete[] data;
	}
	// ******** end of someones code


	error_code err;
	boost::json::value configJson = boost::json::parse(configStr, err);

	if (err) {
		cout << "Error: Cannot parse ./starter.config: \n\t" << err.message() << "\n";
		return Starter_config();
	}

	Starter_config config = boost::json::value_to<Starter_config>(configJson);
	return config;
}

