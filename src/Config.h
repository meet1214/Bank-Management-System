#ifndef CONFIG_H
#define CONFIG_H

#include <fstream>
#include <map>
#include <string>


class Config {
private:
    std::map<std::string, std::string> data;
    
    Config() {}
    
    void parse(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) return;
        
        std::string line;
        while (std::getline(file, line)) {
            // skip empty lines and section headers
            if (line.empty() || line[0] == '[' || line[0] == '#') 
                continue;
            
            // find the '=' and split key=value
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;
            
            std::string key   = line.substr(0,pos);
            std::string value = line.substr(pos+1);
            data[key] = value;
        }
    }

public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    void load(const std::string& filename = "config.ini") {
        parse(filename);
    }

    std::string get(const std::string& key, 
                    const std::string& defaultValue = "") const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultValue;
    }

    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        auto it = data.find(key);
        if (it == data.end()) return defaultValue;
        try { return std::stod(it->second); }
        catch (...) { return defaultValue; }
    }

    int getInt(const std::string& key, int defaultValue = 0) const {
        auto it = data.find(key);
        if (it == data.end()) return defaultValue;
        try { return std::stoi(it->second); }
        catch (...) { return defaultValue; }
    }
};

#endif