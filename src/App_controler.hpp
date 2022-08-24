#pragma once

#include <string>
#include <mutex>
#include <boost/process/child.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "thread.h"
#include "PLC_IO_bridge.hpp"


class App_controler : public Thread {
private:


	std::mutex mutex;
	boost::process::child userApp;
	std::string userFilesRoot;
	std::string userFilesExec;

public:
	void setPath(std::string path) {
		mutex.lock();
		userFilesRoot = path;
		mutex.unlock();
	}

	void setExec(std::string name) {
		mutex.lock();
		userFilesExec = name;
		mutex.unlock();
	}

	bool startApp() {
		bool isSuccess = true;
		mutex.lock();
		try {
			userApp = boost::process::child(userFilesRoot + "/" + userFilesExec);
		}
		catch (...) {
			isSuccess = false;
		}
		mutex.unlock();
		return false;
	}

	bool stopApp() {
		mutex.lock();
		std::error_code err;
		userApp.terminate(err);
		mutex.unlock();
		return err ? true : false;
	}

	bool readFile(std::string path, std::shared_ptr<uint8_t[]>* result, size_t* fileLen) {

		mutex.lock();
		auto root = userFilesRoot;
		mutex.unlock();

		std::fstream file(root + "/" + path, std::ios::in | std::ios::binary);
		
		if (!file.good()) return false;
		// ******* this code is from https://cplusplus.com/reference/istream/istream/read/
		{
			// get length of file
			file.seekg(0, file.end);
			*fileLen = file.tellg();

			// set pos to start
			file.seekg(0, file.beg);

			// read from file and store to string 
			char* data = new char[*fileLen];
			
			*result = std::shared_ptr<uint8_t[]>(new uint8_t[*fileLen]);
			file.read((char*)result->get(), *fileLen);
		}
		// ******** end of someones code

		return true;
	}

	bool saveFile(std::string path, std::shared_ptr<uint8_t[]> data, size_t count) {
		mutex.lock();
		auto root = userFilesRoot;
		mutex.unlock();

		// create directory
		auto p = std::filesystem::path(root + "/" + path).remove_filename();
		std::filesystem::create_directory(p);

		// create file
		std::fstream file(root + "/" + path, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!file.good()) return false;
		
		// save to file
		file.write((char*)data.get(), count);

		return true;
	}


	std::vector<std::string> readDir(std::error_code& err) {
		std::vector<std::string> result;

		mutex.lock();
		auto root = userFilesRoot;
		mutex.unlock();

		auto rootPath = std::filesystem::path(root);
		size_t rootPathSize = rootPath.string().size();

		for (auto pathEntry : std::filesystem::recursive_directory_iterator(root, err)) {
			if (err) return {};

			// ignore non-files
			if (!pathEntry.is_regular_file()) continue;
			
			// get file path and remove 'root' part
			std::string file = pathEntry.path().string().substr(rootPathSize + 1);
			result.push_back(file);
		}

		return result;
	}

private:
	void threadJob() {
		PLC_IO_bridge io_bridge(boost::interprocess::open_or_create);

		std::vector<PLC_IO_Module_Data> newBlocks;
		newBlocks.push_back({});
		io_bridge.set_IO_Blocks(newBlocks);



		while (IsRun()) {
			auto blocks = io_bridge.get_IO_Blocks();
			for (auto b : blocks) {
				std::cout << b.toString() << "\n";
			}
			std::cout << "\n";

			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

	}


};
