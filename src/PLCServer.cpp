#include "PLCServer.h"




void PLC_server_connection_handle::commandFileList(const std::string& cmd){
    std::string path = "./userAppRoot/root";

    std::filesystem::recursive_directory_iterator dir_iter(path);

    boost::json::array result_array;

    for(auto dir_entry : dir_iter){

        std::error_code err;

        // IMPORTANT 
        // time_since_epoch() does not guarantee to return time duration since unix epoch util c++20.
        //
        // TODO: change it later or change c++ version to c++20
        //
        namespace ch = std::chrono;
        auto modification_time_point = ch::clock_cast<ch::system_clock>(dir_entry.last_write_time());
        auto modification_time = ch::duration_cast<ch::seconds>(modification_time_point.time_since_epoch()).count();


        std::string type;
        if(dir_entry.is_directory()) 
            type = "Dir";
        else if(dir_entry.is_regular_file()) 
            type = "File";
        else 
            type = "Other";

        std::wstring_convert<std::codecvt<char32_t,char,std::mbstate_t>,char32_t> convert32;
        
        auto relative_path = std::filesystem::relative(dir_entry.path(), path);
        auto path = convert32.to_bytes(relative_path.u32string());

        boost::json::object dir_entry_js = 
            {
                {"Name", path }, 
                {"Date", modification_time},
                {"Type", type }
            };
        
        result_array.push_back(dir_entry_js);
    }

    boost::json::value response = { {"Cmd",cmd}, {"Result","OK"}, {"DirEntry", result_array} };
    sendJson(response);
}


bool PLC_server_connection_handle::readFileHex(const std::string& path, std::string* hex_file) {

    try {

        std::fstream file(path, std::ios::binary | std::ios::in);

        if (!file.is_open()) {
            return false;
        }

        file.seekg(0,std::ios::end);
        int count = file.tellg();
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

    std::filesystem::path root = "./userAppRoot/root";
    
    std::string name;

    if (data_frame.contains("FileName"))
        if (auto file_name = data_frame.at("FileName").if_string())
            name = *file_name;



    if (name == "") {
        boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","FileName is not specified"} };
        sendJson(response);
        return;
    }

    std::filesystem::path path = root.append(name);

    if (!std::filesystem::exists(path)) {
        boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","File \"" + name + "\" does not exists"} };
        sendJson(response);
        return;
    }

    std::string file_content;
    if (!readFileHex(path.string(), &file_content)) {
        boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Cannot read file"} };
        sendJson(response);
        return;
    }

    boost::json::value response = { {"Cmd",cmd}, {"Result","Ok"}, {"Data",file_content} };
    sendJson(response);


}


bool PLC_server_connection_handle::writeFileHex(const std::string& path,const std::string& hex_str){

    if ( hex_str.size() % 2 != 0){
        return false;
    }

    const char* hex = hex_str.c_str();
    int count = hex_str.size() / 2;


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
    std::filesystem::path root = "./userAppRoot/root";
    
    std::string name;

    if (data_frame.contains("FileName")){
        if (auto file_name = data_frame.at("FileName").if_string()){
            name = *file_name;
        }else{
            boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Field \"FileName\" must be string"} };
            sendJson(response);
            return;
        }
    }else{
        boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Missing field \"FileName\""} };
        sendJson(response);
        return;
    }

    if(name == ""){
        boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","\"FileName\" Cannot be empty string"} };
        sendJson(response);
        return;
    }
    
    std::string data_hex;

    if (data_frame.contains("Data")){
        if (auto data_hex_str = data_frame.at("Data").if_string()){
            data_hex = *data_hex_str;
        }else{
            boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Field \"Data\" must be string"} };
            sendJson(response);
            return;
        }
    }else{
        boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Missing field \"Data\""} };
        sendJson(response);
        return;
    }

    std::filesystem::path path = root;
    path.append(name);
    if (!writeFileHex(path.string(), data_hex)) {
        boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Cound not write file"} };
        sendJson(response);
        return;
    }

    boost::json::value response = { {"Cmd",cmd}, {"Result","OK"} };
    sendJson(response);
    return;

}