#include <fstream>
#include <sstream>

#include "ConfigParser.h"

ConfigParser::ConfigParser(const std::string& config_name)
    : config_name_(config_name)
{}

config_data_type ConfigParser::parse_config() const {
    std::ifstream in(config_name_);

    config_data_type config_data;
    if (!in) {
        // File does not exist
        return config_data;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream stream(line);
        std::string cmd;
        std::string program;
        stream >> cmd;
        stream >> program;
        if (!cmd.empty() && !program.empty()) {
            config_data[cmd] = program;                                                                                                                                                                                                          
        }
    }

    return config_data;
}

