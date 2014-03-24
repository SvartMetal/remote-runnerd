#include <fstream>

#include <boost/algorithm/string.hpp>

#include "ConfigParser.h"

ConfigParser::ConfigParser(const std::string& config_name)
    : config_name_(config_name)
{}

std::vector<std::string> ConfigParser::parse_config() {
    std::ifstream in(config_name_);

    std::vector<std::string> programs;
    if (!in) {
        // File does not exist
        return programs;
    }

    std::string program;
    while (std::getline(in, program)) {
        boost::algorithm::trim(program);
        if (!program.empty()) {
            programs.push_back(program);
        }
    }

    return programs;
}