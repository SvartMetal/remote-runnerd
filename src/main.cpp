#include <iostream>
#include <fstream>

#include <boost/lexical_cast.hpp>

#include "Server.h"
#include "settings.h"

void usage() {
    std::cout << "USAGE: remote-runnerd <timeout>" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        exit(0);
    }

    std::ifstream config(settings::config_file_name);

    if (!config) {
        std::cerr << "Config file does not exist. " << std::endl;
        exit(0);
    }

    try {
        size_t timeout = boost::lexical_cast<size_t>(argv[1]);
        Server server(settings::port, settings::server_thread_pool_size, timeout);

        server.run();
    } catch (const boost::bad_lexical_cast& e) {
        std::cerr << "Bad timeout value. " 
            << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Something bad happened. "
            << e.what() << std::endl;
    }
    return 0;
}
