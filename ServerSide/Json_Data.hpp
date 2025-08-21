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
#include <map>
#include <optional>

#include "../Utils/Helper_Functions.hpp"

enum class DeviceState : uint8_t
{
    OFF = 0,
    ON = 1,
    ERROR = 2,
    UNKNOWN = 255
};

class Json_Device
{
private:
    std::string file_path_;

    int uid;
    std::string name;
    std::string type;
    std::string last_ip;

    std::vector<std::string> settings;
    std::vector<std::string> error_log;
    std::map<std::string, std::string> peripherals;

    std::vector<int> subscribers;

    std::vector<std::string> dirty_data;

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

    static std::string vec_to_string(const std::vector<int> &vec)
    {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < vec.size(); ++i)
        {
            oss << vec[i];
            if (i + 1 < vec.size())
                oss << ",";
        }
        oss << "]";
        return oss.str();
    }

    static std::string trim(const std::string &s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        size_t end = s.find_last_not_of(" \t\r\n");
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
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

    static std::vector<int> string_to_vec_int(const std::string &str)
    {
        std::vector<int> vec;
        std::string num;
        for (char c : str)
        {
            if (isdigit(c))
                num += c;

            else if (!num.empty())
            {
                vec.push_back(std::stoi(num));
                num.clear();
            }
        }
        if (!num.empty())
            vec.push_back(std::stoi(num));
        return vec;
    }

    std::string map_to_string(const std::map<std::string, std::string> &m) const
    {
        std::string result = "[";
        for (auto [peripheral, value] : m)
            result += peripheral + ":" + value + ";";

        if (result.size() > 1)
            result[result.length() - 1] = ']';
        else
            result += ']';
        return result;
    }

    std::map<std::string, std::string> string_to_map(const std::string &str)
    {
        std::map<std::string, std::string> per_map;

        size_t str_begin = str.find('[') + 1;
        size_t str_end = str.find(']');
        if (str_end == std::string::npos || str_begin >= str_end)
            return std::map<std::string, std::string>();

        std::string data = str.substr(str_begin, str_end - str_begin);

        std::vector<std::string> tokens = smh::split_string(data, ';');

        for (auto pair : tokens)
        {
            auto per_val = smh::split_string(pair, ':');
            if (per_val.size() != 2)
                continue;

            per_map[per_val[0]] = per_val[1];
        }

        return per_map;
    }

public:
    bool is_valid;

    Json_Device() = default;

    Json_Device(const std::string &file_path) : file_path_(file_path), state(DeviceState::UNKNOWN), last_contact(0) { is_valid = load(); }

    ~Json_Device() { save_update_last_contact(); };

    void init(const std::string &default_name, uint8_t default_uid = 0, const std::string &default_ip = "0.0.0.0",
              const std::string &default_type = "Unknown")
    {
        uid = default_uid;
        name = default_name;
        type = default_type;
        last_ip = default_ip;

        settings.clear();
        error_log.clear();
        peripherals.clear();
        subscribers.clear();

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

            std::string key = trim(line.substr(0, colon));
            std::string value = trim(line.substr(colon + 1));

            if (key == "\"uid\"")
                uid = std::stoi(value);
            else if (key == "\"name\"")
                name = value.substr(1, value.size() - 3);
            else if (key == "\"type\"")
                type = value.substr(1, value.size() - 3);
            else if (key == "\"last_ip\"")
                last_ip = value.substr(1, value.size() - 3);
            else if (key == "\"settings\"")
                settings = string_to_vec(value);
            else if (key == "\"error_log\"")
                error_log = string_to_vec(value);
            else if (key == "\"peripherals\"")
                peripherals = string_to_map(value);
            else if (key == "\"subscribers\"")
                subscribers = string_to_vec_int(value);
            else if (key == "\"dirty_data\"")
                dirty_data = string_to_vec(value);
            else if (key == "\"last_contact\"")
                last_contact = std::stoull(value);
            else if (key == "\"state\"")
                state = static_cast<DeviceState>(std::stoi(value));
        }

        file.close();
        return true;
    }

    bool save_update_last_contact()
    {
        set_last_contact_now();
        return save();
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
        file << "  \"peripherals\": " << map_to_string(peripherals) << ",\n";
        file << "  \"subscribers\": " << vec_to_string(subscribers) << ",\n";
        file << "  \"dirty_data\": " << vec_to_string(dirty_data) << ",\n";
        file << "  \"last_contact\": " << last_contact << ",\n";
        file << "  \"state\": " << static_cast<int>(state) << "\n";
        file << "}\n";

        file.close();

        return true;
    }

    // ----- Getters -----
    int get_uid() const { return uid; }
    int get_dirty_data_size() { return dirty_data.size(); }

    std::string get_name() const { return name; }
    std::string get_type() const { return type; }
    std::string get_last_ip() const { return last_ip; }
    std::vector<std::string> get_settings() const { return settings; }
    std::vector<std::string> get_dirty_data() const { return dirty_data; }
    std::vector<std::string> get_error_log() const { return error_log; }
    std::map<std::string, std::string> get_peripherals() const { return peripherals; }
    std::vector<int> get_subscribers() const { return subscribers; }
    uint64_t get_last_contact() const { return last_contact; }
    DeviceState get_state() const { return state; }

    // ----- Setters -----
    void set_uid(uint8_t value) { uid = value; }
    void set_name(const std::string &value) { name = value; }
    void set_type(const std::string &value) { type = value; }
    void set_last_ip(const std::string &value) { last_ip = value; }
    void set_settings(const std::vector<std::string> &value) { settings = value; }
    void set_error_log(const std::vector<std::string> &value) { error_log = value; }
    void set_peripherals(const std::map<std::string, std::string> &value) { peripherals = value; }
    void set_subscribers(const std::vector<int> &value) { subscribers = value; }
    void add_subscriber(int uid)
    {
        if (std::find(subscribers.begin(), subscribers.end(), uid) == subscribers.end())
            subscribers.push_back(uid);
    }

    void clear_dirty_data() { dirty_data.clear(); }

    void add_dirty_data(std::string data) { dirty_data.push_back(data); }

    void remove_subscriber(const int &uid)
    {
        subscribers.erase(
            std::remove(subscribers.begin(), subscribers.end(), uid),
            subscribers.end());
    }
    void set_last_contact(uint64_t value) { last_contact = value; }
    void set_state(DeviceState value) { state = value; }
    void set_last_contact_now()
    {
        auto now = std::chrono::system_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        set_last_contact(now_ms);
    }
};

class Json_Server
{
private:
    std::string file_path_;

    std::string ip_address;
    int next_uid;
    std::map<int, std::string> devices;
    std::vector<std::string> error_log;
    uint64_t uptime_start;

    // Helpers for JSON-like map/vector
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

    static std::string map_to_string(const std::map<int, std::string> &m)
    {
        std::ostringstream oss;
        oss << "{";
        size_t i = 0;
        for (const auto &kv : m)
        {
            oss << "\"" << kv.first << "\":\"" << kv.second << "\"";
            if (i + 1 < m.size())
                oss << ",";
            i++;
        }
        oss << "}";
        return oss.str();
    }

    static std::map<int, std::string> string_to_map(const std::string &str)
    {
        std::map<int, std::string> m;
        std::string key, value;
        bool in_key = false, in_value = false;
        bool reading_key = true;
        for (char c : str)
        {
            if (c == '"')
            {
                if (!in_key && !in_value)
                {
                    if (reading_key)
                        in_key = true;
                    else
                        in_value = true;
                }
                else if (in_key)
                {
                    in_key = false;
                    reading_key = false;
                }
                else if (in_value)
                {
                    in_value = false;
                    reading_key = true;
                    if (!key.empty() && !value.empty())
                    {
                        m[std::stoi(key)] = value;
                        key.clear();
                        value.clear();
                    }
                }
            }
            else if (in_key)
                key += c;
            else if (in_value)
                value += c;
        }
        return m;
    }

public:
    Json_Server() {}
    Json_Server(const std::string &file_path) : file_path_(file_path), next_uid(2), uptime_start(0)
    {
        load();
    }

    void init(const std::string &default_ip = "0.0.0.0")
    {
        ip_address = default_ip;
        next_uid = 2;
        devices.clear();
        error_log.clear();
        uptime_start = 0;
        devices[0] = "webs_device_ins";
        devices[5] = "test_device_ins";
        devices[4] = "test_device_out";
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

            if (key == "\"ip_address\"")
                ip_address = value.substr(1, value.size() - 2);
            else if (key == "\"next_uid\"")
                next_uid = std::stoi(value);
            else if (key == "\"devices\"")
                devices = string_to_map(value);
            else if (key == "\"error_log\"")
                error_log = string_to_vec(value);
            else if (key == "\"uptime_start\"")
                uptime_start = std::stoull(value);
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
        file << "  \"ip_address\": \"" << ip_address << "\",\n";
        file << "  \"next_uid\": " << next_uid << ",\n";
        file << "  \"devices\": " << map_to_string(devices) << ",\n";
        file << "  \"error_log\": " << vec_to_string(error_log) << ",\n";
        file << "  \"uptime_start\": " << uptime_start << "\n";
        file << "}\n";

        file.close();
        return true;
    }

    // ----- Getters -----
    std::string get_ip_address() const { return ip_address; }
    int get_next_uid() const { return next_uid; }
    std::map<int, std::string> get_devices() const { return devices; }
    std::vector<std::string> get_error_log() const { return error_log; }
    uint64_t get_uptime_start() const { return uptime_start; }
    std::optional<std::string> try_get_device_by_uid(int uid)
    {
        auto it = devices.find(uid);
        if (it != devices.end())
            return it->second;
        return std::nullopt;
    }

    // ----- Setters -----
    void set_ip_address(const std::string &ip) { ip_address = ip; }
    void set_next_uid(int uid) { next_uid = uid; }
    void set_devices(const std::map<int, std::string> &d) { devices = d; }
    void set_error_log(const std::vector<std::string> &e) { error_log = e; }
    void set_uptime_start(uint64_t value) { uptime_start = value; }

    void add_device(int uid, std::string name)
    {
        devices[uid] = name;
        save();
    }

    // ----- Helpers -----
    int assign_new_uid(const std::string &device_name)
    {
        int uid = next_uid++;
        devices[uid] = device_name;
        save();
        return uid;
    }

    void add_error(const std::string &err)
    {
        error_log.push_back(err);
        save();
    }

    void set_uptime_now()
    {
        auto now = std::chrono::system_clock::now();
        uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        uptime_start = now_ms;
        save();
    }
};
