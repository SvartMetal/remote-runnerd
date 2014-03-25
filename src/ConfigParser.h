#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <vector>
#include <string>

#include "types.h"

class ConfigParser {
public: // constructors

    ConfigParser(const std::string& config_name);

public: // methods
    
    /*
        Returns empty config data on invalid config file.
        This method should not throw any exception.
        Config format:
            <cmd> <program>
            <cmd> <program>
            ... 

        'program' is an executable name for corresponding 'cmd'.
    */
    config_data_type parse_config() const;

private: // fields
    
    std::string config_name_;
};

#endif // CONFIG_PARSER_H