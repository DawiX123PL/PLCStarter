#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <istream>
#include <boost/process.hpp>
#include <boost/process/io.hpp>
#include <sstream>
#include <iostream>
#include <boost/json.hpp>


class AppBuilder{


    struct BuildConfig{
        std::vector<std::string> files;
        std::vector<std::string> includes;
        std::vector<std::string> cpp_flags;
        std::vector<std::string> c_flags;
        std::vector<std::string> ld_flags;

        bool fromString(const std::string& str);
    };


    bool readFile(const std::filesystem::path& path, std::string* file_content){

        std::fstream file(path, std::ios::in | std::ios::binary);

        if(!file.is_open()) return false;
        if(!file.good()) return false;

        file.seekg(0, std::ios::end);
        size_t count = file.tellg();
        file.seekg(0, std::ios::beg);

        std::unique_ptr data_ptr = std::make_unique<char[]>(count);
        char* data = data_ptr.get();

        file.read(data, count);

        *file_content = std::string(data, count);
        if(file.bad()) return false;

        return true;
    }

    std::string flagsToString(const std::vector<std::string>& flag_list){
        std::string flags;
        for(std::string flag: config.cpp_flags){
            flags += " " + flag;
        }
        return flags;
    }


    BuildConfig config;
    std::filesystem::path project_root;

public:


    bool loadBuildConf(std::filesystem::path path){

        project_root = path.parent_path();

        // read config file content
        std::string config_str;
        if(!readFile(path, &config_str)) return false;

        // parse config file
        if(!config.fromString(config_str)) return false;

        return true;
    }


    struct BuildResult {
        int error_code;
        std::string file;
        std::string error_msg;
        BuildResult(int err,std::string _file, std::string msg):error_code(err),file(_file), error_msg(msg) {}
    };


    // TODO:
    //
    // Include directories are not passed to g++ yet.
    // DO this in future.
    //
    // TODO:
    //
    // This function is synchronous GARBAGE.
    // It can easily freeze whole server for significant time.
    // 
    // Replace this with something asynchronous in future !!!!
    // 
    std::vector<BuildResult> build(){



        std::vector<BuildResult> result_list;


        // merge C++/C/LD flags into one string;
        std::string cpp_flags = flagsToString(config.cpp_flags);
        std::string c_flags = flagsToString(config.c_flags);
        std::string ld_flags = flagsToString(config.ld_flags);


        // iterate over files and compile to .o

        namespace bp = boost::process;

        boost::filesystem::path compiler =  bp::search_path("g++");

        for(std::string& file: config.files){

            std::filesystem::path path = project_root / file;
            std::filesystem::path obj_path = project_root / "obj" /  file;

            std::filesystem::create_directories(obj_path.parent_path());

            // IMPORTANT: conversion from path to string may throw exception
            // TODO: replace string with wstring
            std::vector<std::string> flags = config.cpp_flags;
            flags.push_back(path.lexically_normal().string());
            flags.push_back("-c");
            flags.push_back("-o");
            flags.push_back(obj_path.lexically_normal().string() + ".o");

            if(path.extension() == "cpp")     flags.insert(flags.end(), config.cpp_flags.begin(), config.cpp_flags.end() );
            else if(path.extension() == "c")  flags.insert(flags.end(), config.c_flags.begin(), config.c_flags.end() );

            bp::pstream out;
            bp::child compiler_process(compiler, bp::args(flags), bp::std_out > bp::null, bp::std_err > out);

            compiler_process.join();
            int exit_code = compiler_process.exit_code();

            std::stringstream sstream;
            sstream << out.rdbuf();
            std::string error_info = sstream.str();

            std::cout << error_info << "\n\n";

            result_list.push_back(BuildResult(exit_code, file, error_info));
        }


        // link to out.exe
        {
            boost::filesystem::path linker =  bp::search_path("g++");

            std::filesystem::path exec_path = project_root / "build/app.exe";

            std::vector<std::string> ld_flags;
            for (std::string file : config.files) {
                std::filesystem::path file_path = project_root / "obj" / (file + ".o");
                ld_flags.push_back(file_path.lexically_normal().string());
            }

            ld_flags.insert(ld_flags.end(), config.ld_flags.begin(), config.ld_flags.end());
            ld_flags.push_back("-o");
            ld_flags.push_back(exec_path.lexically_normal().string());

            std::filesystem::create_directories(exec_path.parent_path());

            bp::pstream out;
            bp::child compiler_process(linker, bp::args(ld_flags), bp::std_out > bp::null, bp::std_err > out);

            compiler_process.join();
            int exit_code = compiler_process.exit_code();

            std::stringstream sstream;
            sstream << out.rdbuf();
            std::string error_info = sstream.str();

            std::cout << error_info << "\n\n";

            result_list.push_back(BuildResult(exit_code, "", error_info));
        }
        return result_list;
    }




};



void tag_invoke( const boost::json::value_from_tag&, boost::json::value& js, AppBuilder::BuildResult build_result);