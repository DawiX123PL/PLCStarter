#pragma once

#include <fstream>
#include <string>
#include <iostream>


class Gpio{

private:

    size_t pin_nr;
    std::string dir_file_path;
    std::string value_file_path;


    inline bool write_to_file(const char* const path, const std::string& data){
        std::fstream file(path, std::ios::out);
        if(file.bad()) return false;

        file.write(data.c_str(), data.size());
        return true;
    }

    inline bool write_to_file(const char* const path, const char* const data, int count){
        std::fstream file(path, std::ios::out);
        if(file.bad()) return false;

        file.write(data, count);
        return true;
    }

    inline bool read_from_file(const char* const path, char* data, int* count){
        std::fstream file(path, std::ios::in);

        if(file.bad()) return false;
        *count = file.read(data, *count).gcount();

        return true;
    }

public:
    enum class Dir{
        INPUT,
        OUTPUT,
    };

    enum class Level{
        LOW,
        HIGH,
    };

    inline Gpio(Dir dir, size_t nr): pin_nr(nr){
        write_to_file("/sys/class/gpio/export", std::to_string(pin_nr));
        dir_file_path = "/sys/class/gpio/gpio" + std::to_string(pin_nr) + "/direction";
        value_file_path = "/sys/class/gpio/gpio" + std::to_string(pin_nr) + "/value";


        if(dir == Dir::INPUT){
            const char str[] = "in";
            write_to_file(dir_file_path.c_str(),str, sizeof(str)-1 );
        }else if(dir == Dir::OUTPUT){
            const char str[] = "out";
            write_to_file(dir_file_path.c_str(),str, sizeof(str)-1 );
        }
    }


    inline ~Gpio(){
        write_to_file("/sys/class/gpio/unexport", std::to_string(pin_nr));
    }


    inline void write(Level level){
        if(level == Level::HIGH){
            const char str[] = "1";
            write_to_file(value_file_path.c_str(),str, sizeof(str) );
        }else if(level == Level::LOW){
            const char str[] = "0";
            write_to_file(value_file_path.c_str(),str, sizeof(str) );
        }
    }

    inline Level read(){
        char file_content [3];
        int count = sizeof(file_content)-1;
        bool isOk = read_from_file(value_file_path.c_str(), file_content, &count);
        if(isOk){ 
            return file_content[0] == '0' ? Level::LOW : Level::HIGH;
        }else{
            return Level::LOW;
        }
    }

};


class GpioIn{

    Gpio gpio;

public:
    GpioIn(size_t nr):gpio(Gpio::Dir::INPUT, nr){};

    Gpio::Level read(){
        return gpio.read();
    }

    bool read_bool(){
        return gpio.read() == Gpio::Level::HIGH;
    }

};

class GpioOut{

    Gpio gpio;

public:
    GpioOut(size_t nr):gpio(Gpio::Dir::OUTPUT, nr){};

    void write(Gpio::Level level){
        gpio.write(level);
    }

    void write(bool level){
        gpio.write(level ? Gpio::Level::HIGH : Gpio::Level::LOW);
    }

};
