#include "App_controler.hpp"
#include "gpio.hpp"



void App_controler::threadJob() {

    std::unique_ptr<boost::process::child> app;

    PLC_IO_bridge io_bridge;	
    int i = 0;

#if USE_PHYSICAL_GPIO == 1

    GpioIn in_phys[] = { 7, 8, 25, 24, 23, 18, 2, 3, 4, 17 };
    GpioOut out_phys[] = { 26, 19, 21, 20, 16, 12, 10, 9, 11, 13 };

    std::cout << "Setting up GPIO pins\n";

    const int in_phys_size = sizeof(in_phys)/sizeof(in_phys[0]);
    const int out_phys_size = sizeof(out_phys)/sizeof(out_phys[0]);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // export pins
    for (int i = 0; i < in_phys_size; i++)
        in_phys[i].Export();

    for (int i = 0; i < out_phys_size; i++)
        out_phys[i].Export();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Set up GPIO direction\n";
    // set direction
    for (int i = 0; i < in_phys_size; i++)
        in_phys[i].SetDir();

    for (int i = 0; i < out_phys_size; i++)
        out_phys[i].SetDir();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // clear output pins
    for (int i = 0; i < out_phys_size; i++) {
        out_phys[i].write(false);
    }
#endif



    while(IsRun()){

        
        if(app_status_mutex.try_lock()){
            // locked
            
            bool start_app = app_status == AppStatus::START;
            bool stop_app = app_status == AppStatus::STOP;
            if(start_app) app_status = AppStatus::RUNNING;
            if(stop_app) app_status = AppStatus::STOPPED;
            app_status_mutex.unlock();

            if( start_app ){
                
                std::error_code err;

                app = std::make_unique<boost::process::child>(
                    app_path.lexically_normal().wstring(),
                    boost::process::std_out > stdout, 
                    boost::process::std_err > boost::process::null, 
                    boost::process::std_in < boost::process::null,
                    err
                    );

                if(err){
                    app_status = AppStatus::STOP;
                    continue;
                }

                // reset registers before app is started
                PLC_io_module* io_module = io_bridge.getIOModulesAdress();
                PLC_io_tag* io_tags = io_bridge.getIOTagsAdress();
                io_module->input = 0;
                io_module->output = 0;

            }

            if( stop_app ){
                std::error_code err;
                if(app) app->terminate(err);

                // reset registers after app is terminated
                PLC_io_module* io_module = io_bridge.getIOModulesAdress();
                PLC_io_tag* io_tags = io_bridge.getIOTagsAdress();
                io_module->input = 0;
                io_module->output = 0;

#if USE_PHYSICAL_GPIO == 1
                for (int i = 0; i < out_phys_size; i++) {
                    out_phys[i].write(false);
                }
#endif
            }

        }

        if(app && !app->running()){
            if(app_status_mutex.try_lock()){
                app_status = AppStatus::STOPPED;
                app.reset();
                app_status_mutex.unlock();
            }
        }

        if(app && app->running()){

            PLC_io_module* io_module = io_bridge.getIOModulesAdress();
            PLC_io_tag* io_tags = io_bridge.getIOTagsAdress();


            std::bitset<32> out = io_module->output;
            std::bitset<32> in = 0;
            // std::bitset<32> in = io_module->input;


#if USE_PHYSICAL_GPIO == 1
            for (int i = 0; i < in_phys_size; i++) {
                in[i] = !(in_phys[i].read_bool());
            }

            for (int i = 0; i < out_phys_size; i++) {
                out_phys[i].write(out[i]);
            }
#endif

            io_module->input = in.to_ullong();


            auto delay = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);
            io_bridge.start_loop(delay);
            PLC_IO_bridge::wait_result err = io_bridge.wait_until_loop_finished_or(delay);


            if (err == PLC_IO_bridge::wait_result::OK) {
                std::cout << "OK  - " << i++ << " IN:"<< in << " OUT:" << out << "\n";
            }
            else {
                std::cout << "ERR - " << i++ << "\n";
            }
        }




    }

}


