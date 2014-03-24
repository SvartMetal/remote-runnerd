#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <vector>
#include <string>

class ConfigParser {
public: // constructors

    ConfigParser(const std::string& config_name);

public: // methods
    
    std::vector<std::string> parse_config();

private: // fields
    
    std::string config_name_;
};

#endif // CONFIG_PARSER_H