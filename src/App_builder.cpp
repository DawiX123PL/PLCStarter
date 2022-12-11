
#include "App_builder.hpp"

#include <boost/json.hpp>



std::filesystem::path AppBuilder::include_dir;
std::filesystem::path AppBuilder::library_path;



void tag_invoke( const boost::json::value_from_tag&, boost::json::value& js, AppBuilder::BuildResult build_result){
    js = {
            {"File", build_result.file},
            {"ExitCode",build_result.error_code},
            {"ErrorMsg",build_result.error_msg}
        };
}



bool AppBuilder::BuildConfig::fromString(const std::string& str){


    files.clear();
    includes.clear();
    cpp_flags.clear();
    c_flags.clear();
    ld_flags.clear();


    namespace js = boost::json;

    std::error_code err;
    const js::value json_val = js::parse(str, err);
    if(err) return false;


    if(!json_val.is_object()) return false;
    const js::object& json_obj = json_val.as_object();


    auto tryConvert = 
        [json_obj]
        ( const char* const name, std::vector<std::string> *array )
        {
            if(json_obj.contains(name)){
                try{
                    *array = js::value_to<std::vector<std::string>>(json_obj.at(name));
                }
                catch(...){

                }
            }
        };



    tryConvert("Files", &files);
    tryConvert("Includes", &includes);
    tryConvert("CPP_flags", &cpp_flags);
    tryConvert("C_flags", &c_flags);
    tryConvert("LD_flags", &ld_flags);


    return true;

}









