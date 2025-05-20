#pragma once
#include "Platform.h"

#ifdef ARDUINO
#include <Preferences.h>
class SimpleStorage {
    Preferences prefs;
public:
    bool begin(const char* ns) { return prefs.begin(ns, false); }
    float getFloat(const char* key, float def) { return prefs.getFloat(key, def); }
    void putFloat(const char* key, float value) { prefs.putFloat(key, value); }
};
#else
#include <unordered_map>
#include <fstream>
#include <sstream>

class SimpleStorage {
    std::string file;
    std::unordered_map<std::string, float> data;
    void load() {
        if (file.empty()) return;
        std::ifstream in(file);
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            std::string key; float value;
            if (iss >> key >> value) data[key] = value;
        }
    }
    void save() const {
        if (file.empty()) return;
        std::ofstream out(file);
        for (const auto& kv : data) out << kv.first << " " << kv.second << "\n";
    }
public:
    bool begin(const char* ns) {
        file = std::string(ns) + ".store";
        load();
        return true;
    }
    float getFloat(const char* key, float def) {
        auto it = data.find(key);
        return it == data.end() ? def : it->second;
    }
    void putFloat(const char* key, float value) {
        data[key] = value;
        save();
    }
};
#endif
