#include <filesystem>
#include <iostream>

#include "Defines.hpp"

namespace fs = std::filesystem;

namespace smh
{
    class Smh_File_Helper
    {
    private:
        fs::path srv_top_dir_path_;
        fs::path srv_inside_device_path_;
        fs::path srv_outside_device_path_;

        bool throw_errors_;

        bool init_with_errors() // File helper initialization that will throw errors if a directory does not exist
        {
            if (!fs::exists(srv_top_dir_path_))
            {
                std::cout << "Server Top Directory \"" << srv_top_dir_path_ << "\" Does not exist." << std::endl;
                return false;
            }

            if (!fs::exists(srv_outside_device_path_) || !fs::exists(srv_inside_device_path_))
            {
                std::cout << "Server directories for device data are invalid" << std::endl;
                return false;
            }

            return true;
        }

        void init_and_create() // File helper initialization that will create the filesystem if it does not exist
        {
            fs::create_directories(srv_top_dir_path_);
            fs::create_directories(srv_outside_device_path_);
            fs::create_directories(srv_inside_device_path_);
        }

    public:
        Smh_File_Helper(bool throw_errors, std::string top_dir_path = SMH_SERVER_DIR_PATH)
            : srv_top_dir_path_(top_dir_path), throw_errors_(throw_errors)
        {
            if (srv_top_dir_path_ == SMH_SERVER_DIR_PATH)
            {
                srv_inside_device_path_ = SMH_SERVER_IN_DEVICES_PATH;
                srv_outside_device_path_ = SMH_SERVER_OUT_DEVICES_PATH;
            }
            else
            {
                srv_inside_device_path_ = srv_top_dir_path_ / "inside";
                srv_outside_device_path_ = srv_top_dir_path_ / "outside";
            }

            if (throw_errors)
                init_with_errors();
            else
                init_and_create();
        }
        ~Smh_File_Helper() {}
    };
}