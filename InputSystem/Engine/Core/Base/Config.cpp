// Engine/Core/Base/Config.cpp
// NEW FILE - Implementation for the Config system
#include "Types.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Engine {
namespace Core {

    // ========================================
    // Config Implementation
    // ========================================

    void Config::setBool(const std::string& key, bool value) {
        values_[key] = value ? "true" : "false";
    }

    void Config::setInt(const std::string& key, int32 value) {
        values_[key] = std::to_string(value);
    }

    void Config::setFloat(const std::string& key, float32 value) {
        values_[key] = std::to_string(value);
    }

    void Config::setString(const std::string& key, const std::string& value) {
        values_[key] = value;
    }

    bool Config::getBool(const std::string& key, bool defaultValue) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return defaultValue;
        }

        const std::string& value = it->second;
        return (value == "true" || value == "1" || value == "yes" || value == "on");
    }

    int32 Config::getInt(const std::string& key, int32 defaultValue) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return defaultValue;
        }

        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }

    float32 Config::getFloat(const std::string& key, float32 defaultValue) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return defaultValue;
        }

        try {
            return std::stof(it->second);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }

    std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
        auto it = values_.find(key);
        if (it == values_.end()) {
            return defaultValue;
        }
        return it->second;
    }

    bool Config::loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        values_.clear();
        std::string line;

        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            // Find the '=' separator
            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos) {
                continue;
            }

            // Extract key and value
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Remove quotes if present
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            values_[key] = value;
        }

        return true;
    }

    bool Config::saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << "# Engine Configuration File\n";
        file << "# Generated automatically\n\n";

        for (const auto& [key, value] : values_) {
            // Quote string values that contain spaces
            if (value.find(' ') != std::string::npos) {
                file << key << "=\"" << value << "\"\n";
            } else {
                file << key << "=" << value << "\n";
            }
        }

        return file.good();
    }

    void Config::clear() {
        values_.clear();
    }

} // namespace Core
} // namespace Engine