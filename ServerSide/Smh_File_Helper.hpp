#pragma once

#include <filesystem>
#include <iostream>
#include <fstream>

#include "../Utils/Defines.hpp"
#include "Json_Data.hpp"

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
                std::cout << "Server Top Directory " << srv_top_dir_path_ << " Does not exist." << std::endl;
                return false;
            }
            else
                std::cout << "Server top directory set to: " << srv_top_dir_path_ << std::endl;

            if (!fs::exists(srv_outside_device_path_) || !fs::exists(srv_inside_device_path_))
            {
                std::cout << "Server directories for device data are invalid" << std::endl;
                return false;
            }
            else
                std::cout << "Device directories set to:\n"
                          << srv_outside_device_path_ << "\nAnd:\n"
                          << srv_inside_device_path_;

            return true;
        }

        void init_and_create() // File helper initialization that will create the filesystem if it does not exist
        {
            fs::create_directories(srv_top_dir_path_);
            fs::create_directories(srv_outside_device_path_);
            fs::create_directories(srv_inside_device_path_);

            std::cout << "Server top directory set to: " << srv_top_dir_path_.generic_string() << std::endl;
            std::cout << "Device directories set to:\n\t"
                      << srv_outside_device_path_.generic_string() << "\n\t"
                      << srv_inside_device_path_.generic_string() << std::endl;
        }

    public:
        Smh_File_Helper(bool throw_errors, std::string top_dir_path)
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

        bool device_file_exists(std::string device_name)
        {
            return fs::exists(fs::path(srv_inside_device_path_) / (device_name + ".json")) || fs::exists(fs::path(srv_outside_device_path_) / (device_name + ".json"));
        }

        bool create_inside_device_file(std::string device_name)
        {
            std::ofstream file(srv_inside_device_path_.string() + ("/" + device_name + ".json"));
            if (!file)
            {
                std::cerr << "Failed to create file for " << device_name;
                return false;
            }
            return true;
        }

        bool create_outside_device_file(std::string device_name)
        {
            std::ofstream file(srv_outside_device_path_.string() + ("/" + device_name + ".json"));
            if (!file)
            {
                std::cerr << "Failed to create file for " << device_name;
                return false;
            }
            return true;
        }

        std::string get_full_path_to_device_file(const std::string &device_name)
        {
            fs::path inside_path = fs::path(srv_inside_device_path_) / (device_name + ".json");
            fs::path outside_path = fs::path(srv_outside_device_path_) / (device_name + ".json");

            if (fs::is_regular_file(inside_path))
                return inside_path.string();
            else if (fs::is_regular_file(outside_path))
                return outside_path.string();

            return "";
        }

        bool create_and_init_device_file(std::string device_name, Json_Device &data)
        {

            if (device_file_exists(device_name))
            {
                get_device_json(device_name, data);
                return true;
            }

            if (strstr(device_name.c_str(), "out")) // outside device
                create_outside_device_file(device_name);
            else if (strstr(device_name.c_str(), "ins")) // inside device
                create_inside_device_file(device_name);
            else
                std::cout << "Could not deduce ins out out type in: " << device_name << std::endl;

            std::string device_file_path = get_full_path_to_device_file(device_name);
            if (device_file_path == "")
                return false;

            data = Json_Device(device_file_path);
            data.init(device_name);

            data.set_last_contact_now();
            data.save();

            return true;
        }

        bool create_and_init_server_file(Json_Server &data)
        {
            std::string srv_json_path = fs::path(srv_top_dir_path_ / "server.json");

            std::ofstream file(srv_json_path);
            if (!file)
            {
                std::cerr << "Failed to create server json file\n";
                return false;
            }
            data = Json_Server(srv_json_path);
            data.init(DEFAULT_SERVER_IP);
            data.save();

            return true;
        }

        bool create_and_init_website_file(Json_Website &data)
        {
            std::string webs_json_path = fs::path(srv_top_dir_path_ / "website_data.json");

            std::ofstream file(webs_json_path);
            if (!file)
            {
                std::cerr << "Failed to create server json file\n";
                return false;
            }
            data = Json_Website(webs_json_path);
            data.init();
            data.save();

            return true;
        }

        bool website_json_exists()
        {
            std::string srv_json_path = fs::path(srv_top_dir_path_ / "website_data.json");
            return fs::exists(srv_json_path);
        }

        bool server_json_exists()
        {
            std::string srv_json_path = fs::path(srv_top_dir_path_ / "server.json");
            return fs::exists(srv_json_path);
        }

        Json_Server get_server_json()
        {
            std::string srv_json_path = fs::path(srv_top_dir_path_ / "server.json");

            return Json_Server(srv_json_path);
        }

        Json_Website get_website_json()
        {
            std::string srv_json_path = fs::path(srv_top_dir_path_ / "website_data.json");

            return Json_Website(srv_json_path);
        }

        void get_device_json(std::string device_name, Json_Device &device)
        {
            std::string device_file_path = get_full_path_to_device_file(device_name);
            if (device_file_path == "")
                return;

            device = Json_Device(device_file_path);
        }
    };
}