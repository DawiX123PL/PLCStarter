#include "PLCServer.h"

#include <boost/locale/encoding_utf.hpp>


void PLC_server_connection_handle::commandFileList(const std::string& cmd){

    std::filesystem::recursive_directory_iterator dir_iter(config.user_app_root);

    boost::json::array result_array;

    for(auto dir_entry : dir_iter){

        // IMPORTANT
        // time_since_epoch() does not guarantee to return time duration since unix epoch util c++20.
        //
        // TODO: change it later or change c++ version to c++20
        //
        //                    !!!!! MORE IMPORTANT !!!!!
        //               CODE BELOW DOES NOT COMPILE ON LINUX
        //           + THIS IS GARBAGE CODE - DONT USE IT. PERIOD.
        //
        // // namespace ch = std::chrono;
        // // auto modification_time_point = ch::clock_cast<ch::system_clock>(dir_entry.last_write_time());
        // // auto modification_time = ch::duration_cast<ch::seconds>(modification_time_point.time_since_epoch()).count();

        boost::system::error_code err1;
        boost::filesystem::path p = dir_entry.path().wstring();
        std::time_t modification_time = boost::filesystem::last_write_time(p, err1);


        std::string type;
        if(dir_entry.is_directory()) 
            type = "Dir";
        else if(dir_entry.is_regular_file()) 
            type = "File";
        else 
            type = "Other";


        auto relative_path = std::filesystem::relative(dir_entry.path(), config.user_app_root);
        
        // THIS CODE WONT COMPILE ON LINUX 
        // // std::wstring_convert<std::codecvt<char32_t,char,std::mbstate_t>,char32_t> convert32;
        // // auto path = convert32.to_bytes(relative_path.u32string());

        std::string path = boost::locale::conv::utf_to_utf<char,char32_t>(relative_path.u32string());

        // THIS WONT COMPILE ON LINUX ALSO
        // // boost::json::object dir_entry_js = 
        // //     {
        // //         {"Name", path }, 
        // //         {"Date", modification_time},
        // //         {"Type", type }
        // //     };

        boost::json::object dir_entry_js;
        dir_entry_js["Name"] = path;
        dir_entry_js["Date"] = modification_time;
        dir_entry_js["Type"] = type;

        
        result_array.push_back(dir_entry_js);
    }

    sendJson(jsonResponseOK(cmd,"DirEntry", result_array));
}


bool PLC_server_connection_handle::readFileHex(const std::string& path, std::string* hex_file) {

    try {

        std::fstream file(path, std::ios::binary | std::ios::in);

        if (!file.is_open()) {
            return false;
        }

        file.seekg(0,std::ios::end);
        size_t count = file.tellg();
        file.seekg(0,std::ios::beg);

        std::unique_ptr buf_unique = std::make_unique<char[]>(count); // not using 'new' to prevent memory leaks
        char* buf = buf_unique.get();

        file.read(buf, count);

        std::unique_ptr hex_buf_unique = std::make_unique<char[]>(count*2); // not using 'new' to prevent memory leaks
        char* hex_buf = hex_buf_unique.get();


        for (int i = 0; i < count; i++) {
        
            char lower = buf[i] & 0x0f;
            char higher = buf[i] >> 4;

            hex_buf[i * 2] = higher < 10 ? '0' + higher : 'a' - 0xa + higher;
            hex_buf[i * 2 + 1] = lower < 10 ? '0' + lower : 'a' - 0xa + lower;
        }

        *hex_file = std::string(hex_buf, count * 2);

    }
    catch (...) {
        return false;
    }


    return true;
}


void PLC_server_connection_handle::commandFileRead(boost::json::object data_frame, const std::string& cmd) {
  
    std::string name;

    if (data_frame.contains("FileName"))
        if (auto file_name = data_frame.at("FileName").if_string())
            name = *file_name;



    if (name == "") {
        sendJson(jsonResponseERR(cmd, "FileName is not specified"));
        return;
    }

    std::filesystem::path path = config.user_app_root / name;

    if (!std::filesystem::exists(path)) {
        sendJson(jsonResponseERR(cmd, "File \"" + name + "\" does not exists"));
        return;
    }

    std::string file_content;
    if (!readFileHex(config.user_app_root.string(), &file_content)) {
        sendJson(jsonResponseERR(cmd, "Cannot read file"));
        return;
    }

    sendJson(jsonResponseOK(cmd, "Data", file_content));

}


bool PLC_server_connection_handle::writeFileHex(const std::string& path,const std::string& hex_str){

    if ( hex_str.size() % 2 != 0){
        return false;
    }

    const char* hex = hex_str.c_str();
    size_t count = hex_str.size() / 2;


    std::unique_ptr data_unique_ptr = std::make_unique<char[]>(count);
    char* data = data_unique_ptr.get();

    for(int i = 0; i < count; i++){
        
        char lower = hex[i * 2 + 1];
        char higher = hex[i * 2];;

        char byte = 0;
        if(higher >= '0' && higher <= '9') byte = higher - '0';
        else if(higher >= 'a' && higher <= 'f') byte = higher - 'a' + 0xa;
        else if(higher >= 'A' && higher <= 'F') byte = higher - 'A' + 0xa;
        else higher += 0;

        byte <<= 4;
        if(lower >= '0' && lower <= '9') byte += lower - '0';
        else if(lower >= 'a' && lower <= 'f') byte += lower - 'a' + 0xa;
        else if(lower >= 'A' && lower <= 'F') byte += lower - 'A' + 0xa;
        else byte += 0;

        data[i] = byte;
    }

    // send data to file;

    try{


        std::fstream file(path, std::ios::out | std::ios::binary);

        if(!file.is_open()) return false;
        if(file.bad()) return false;
                    
        file.write(data, count);


    }catch(...){
        return false;
    }

    return true;
}


void PLC_server_connection_handle::commandFileWrite(boost::json::object data_frame, const std::string& cmd){
    
    std::string name;

    if (data_frame.contains("FileName")){
        if (auto file_name = data_frame.at("FileName").if_string()){
            name = *file_name;
        }else{
            sendJson(jsonResponseERR(cmd, "Field \"FileName\" must be string"));
            return;
        }
    }else{
        sendJson(jsonResponseERR(cmd, "Missing field \"FileName\""));
        return;
    }

    if(name == ""){
        sendJson(jsonResponseERR(cmd, "\"FileName\" Cannot be empty string"));
        return;
    }
    
    std::string data_hex;

    if (data_frame.contains("Data")){
        if (auto data_hex_str = data_frame.at("Data").if_string()){
            data_hex = *data_hex_str;
        }else{
            sendJson(jsonResponseERR(cmd, "Field \"Data\" must be string"));
            return;
        }
    }else{
        sendJson(jsonResponseERR(cmd, "Missing field \"Data\""));
        return;
    }

    std::filesystem::path path = config.user_app_root / name;


    if (!writeFileHex(path.string(), data_hex)) {
        sendJson(jsonResponseERR(cmd, "Cound not write file"));
        return;
    }

    boost::json::value response = { {"Cmd",cmd}, {"Result","OK"} };
    sendJson(jsonResponseOK(cmd));
    return;

}
