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

    static std::filesystem::path library_path;
    static std::filesystem::path include_dir;



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
            flags.push_back("-I" + include_dir.lexically_normal().string()); // add include directory with file PLC_app.hpp inside



            if(path.extension() == "cpp")     flags.insert(flags.end(), config.cpp_flags.begin(), config.cpp_flags.end() );
            else if(path.extension() == "c")  flags.insert(flags.end(), config.c_flags.begin(), config.c_flags.end() );

            // filter out empty flags
            std::vector<std::string> flags_final;
            for(std::string& f: flags)
                if(!f.empty())
                    flags_final.emplace_back(f);


            bp::pstream error_stream;
            bp::pstream out_stream;
            bp::child compiler_process(compiler, bp::args(flags_final), bp::std_out > out_stream, bp::std_err > error_stream, bp::std_in < bp::null);

            compiler_process.join();
            int exit_code = compiler_process.exit_code();
            
            std::string error_str;
            std::string out_str;

            {
                std::stringstream sstream;
                sstream << error_stream.rdbuf();
                error_str = sstream.str();
            }

            {
                std::stringstream sstream;
                sstream << out_stream.rdbuf();
                out_str = sstream.str();
            }

            result_list.push_back(BuildResult(exit_code, file, out_str + error_str));
        }

        // check if compilation of any cpp file failed
        // and not link then
        bool is_any_failed = false;
        for(BuildResult result: result_list)
            if(result.error_code != 0)
                is_any_failed = true;


        // link to out.exe
        if(!is_any_failed){

            boost::filesystem::path linker =  bp::search_path("g++");

            std::vector<std::string> ld_flags;
            for (std::string file : config.files) {
                std::filesystem::path file_path = project_root / "obj" / (file + ".o");
                ld_flags.push_back(file_path.lexically_normal().string());
            }

            std::filesystem::path exec_path = project_root / "build/app.exe";

            ld_flags.insert(ld_flags.end(), config.ld_flags.begin(), config.ld_flags.end());
            ld_flags.push_back("-o");
            ld_flags.push_back(exec_path.lexically_normal().string());
            ld_flags.push_back("-L" + library_path.lexically_normal().string());
            ld_flags.push_back("-lPLCAppLib");

#ifdef __unix__ 
            ld_flags.push_back("-lrt"); // link linux runtime library
#endif

            // filter out empty flags
            std::vector<std::string> ld_flags_final;
            for(std::string& f: ld_flags)
                if(!f.empty())
                    ld_flags_final.emplace_back(f);

            std::filesystem::create_directories(exec_path.parent_path());

            bp::pstream error_stream;
            bp::pstream out_stream;
            bp::child compiler_process(linker, bp::args(ld_flags_final), bp::std_out > out_stream, bp::std_err > error_stream, bp::std_in < bp::null);

            compiler_process.join();
            int exit_code = compiler_process.exit_code();

            std::string error_str;
            std::string out_str;

            {
                std::stringstream sstream;
                sstream << error_stream.rdbuf();
                error_str = sstream.str();
            }

            {
                std::stringstream sstream;
                sstream << out_stream.rdbuf();
                out_str = sstream.str();
            }

            result_list.push_back(BuildResult(exit_code, "build.conf", out_str + error_str));

        }
        return result_list;
    }




};



void tag_invoke( const boost::json::value_from_tag&, boost::json::value& js, AppBuilder::BuildResult build_result);