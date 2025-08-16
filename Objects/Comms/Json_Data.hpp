#pragma once

#include <chrono>
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

enum class DeviceState : uint8_t
{
    OFF = 0,
    ON = 1,
    ERROR = 2,
    UNKNOWN = 255
};

class Json_Data
{
private:
    std::string file_path_;

    int uid;
    std::string name;
    std::string type;
    std::string last_ip;

    std::vector<std::string> settings;
    std::vector<std::string> error_log;
    std::vector<std::string> peripherals;
    std::vector<std::string> subscribed_to_devices;

    uint64_t last_contact;
    DeviceState state;

    // Helpers for simple JSON-like parsing
    static std::string vec_to_string(const std::vector<std::string> &vec)
    {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < vec.size(); ++i)
        {
            oss << "\"" << vec[i] << "\"";
            if (i + 1 < vec.size())
                oss << ",";
        }
        oss << "]";
        return oss.str();
    }

    static std::vector<std::string> string_to_vec(const std::string &str)
    {
        std::vector<std::string> vec;
        std::string tmp;
        bool in_string = false;
        for (char c : str)
        {
            if (c == '"')
            {
                in_string = !in_string;
                if (!in_string && !tmp.empty())
                {
                    vec.push_back(tmp);
                    tmp.clear();
                }
            }
            else if (in_string)
                tmp += c;
        }
        return vec;
    }

public:
    Json_Data() = default;

    Json_Data(const std::string &file_path) : file_path_(file_path), state(DeviceState::UNKNOWN), last_contact(0)
    {
        load();
    }

    ~Json_Data() = default;

    void init(uint8_t default_uid, const std::string &default_name, const std::string &default_ip = "0.0.0.0",
              const std::string &default_type = "Unknown")
    {
        uid = default_uid;
        name = default_name;
        type = default_type;
        last_ip = default_ip;

        settings.clear();
        error_log.clear();
        peripherals.clear();
        subscribed_to_devices.clear();

        last_contact = 0;
        state = DeviceState::UNKNOWN;
    }

    bool load()
    {
        std::ifstream file(file_path_);
        if (!file.is_open())
            return false;

        std::string line;
        while (std::getline(file, line))
        {
            auto colon = line.find(':');
            if (colon == std::string::npos)
                continue;

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            key.erase(remove_if(key.begin(), key.end(), isspace), key.end());
            value.erase(remove_if(value.begin(), value.end(), isspace), value.end());

            if (key == "\"uid\"")
            {
                uid = std::stoi(value);
            }
            else if (key == "\"name\"")
                name = value.substr(1, value.size() - 2);
            else if (key == "\"type\"")
                type = value.substr(1, value.size() - 2);
            else if (key == "\"last_ip\"")
                last_ip = value.substr(1, value.size() - 2);
            else if (key == "\"settings\"")
                settings = string_to_vec(value);
            else if (key == "\"error_log\"")
                error_log = string_to_vec(value);
            else if (key == "\"peripherals\"")
                peripherals = string_to_vec(value);
            else if (key == "\"subscribed_to_devices\"")
                subscribed_to_devices = string_to_vec(value);
            else if (key == "\"last_contact\"")
                last_contact = std::stoull(value);
            else if (key == "\"state\"")
                state = static_cast<DeviceState>(std::stoi(value));
        }

        file.close();
        return true;
    }

    bool save() const
    {
        std::ofstream file(file_path_);
        if (!file.is_open())
            return false;

        file << "{\n";
        file << "  \"uid\": " << (int)uid << ",\n";
        file << "  \"name\": \"" << name << "\",\n";
        file << "  \"type\": \"" << type << "\",\n";
        file << "  \"last_ip\": \"" << last_ip << "\",\n";
        file << "  \"settings\": " << vec_to_string(settings) << ",\n";
        file << "  \"error_log\": " << vec_to_string(error_log) << ",\n";
        file << "  \"peripherals\": " << vec_to_string(peripherals) << ",\n";
        file << "  \"subscribed_to_devices\": " << vec_to_string(subscribed_to_devices) << ",\n";
        file << "  \"last_contact\": " << last_contact << ",\n";
        file << "  \"state\": " << static_cast<int>(state) << "\n";
        file << "}\n";

        file.close();
        return true;
    }

    // ----- Getters -----
    int get_uid() const { return uid; }
    std::string get_name() const { return name; }
    std::string get_type() const { return type; }
    std::string get_last_ip() const { return last_ip; }
    std::vector<std::string> get_settings() const { return settings; }
    std::vector<std::string> get_error_log() const { return error_log; }
    std::vector<std::string> get_peripherals() const { return peripherals; }
    std::vector<std::string> get_subscribed_to_devices() const { return subscribed_to_devices; }
    uint64_t get_last_contact() const { return last_contact; }
    DeviceState get_state() const { return state; }

    // ----- Setters -----
    void set_uid(uint8_t value) { uid = value; }
    void set_name(const std::string &value) { name = value; }
    void set_type(const std::string &value) { type = value; }
    void set_last_ip(const std::string &value) { last_ip = value; }
    void set_settings(const std::vector<std::string> &value) { settings = value; }
    void set_error_log(const std::vector<std::string> &value) { error_log = value; }
    void set_peripherals(const std::vector<std::string> &value) { peripherals = value; }
    void set_subscribed_to_devices(const std::vector<std::string> &value) { subscribed_to_devices = value; }
    void set_last_contact(uint64_t value) { last_contact = value; }
    void set_state(DeviceState value) { state = value; }
};
