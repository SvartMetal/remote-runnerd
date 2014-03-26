#include <csignal>
#include <iostream>
#include <fstream>

#include <boost/lexical_cast.hpp>

#include "Server.h"
#include "settings.h"

std::shared_ptr<Server> server_ptr;

void child_exit_handler(int signum, siginfo_t* info, void* context) {
    server_ptr->process_child_exit(info->si_pid);
}

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
        exit(1);
    }

    try {
        size_t timeout = boost::lexical_cast<size_t>(argv[1]);
        server_ptr = std::make_shared<Server>(
            settings::port, settings::server_thread_pool_size, timeout);

        // Signal handler initializing
        struct sigaction sa;
        sa.sa_sigaction = &child_exit_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGCHLD, &sa, nullptr);

        server_ptr->run();
        exit(0);
    } catch (const boost::bad_lexical_cast& e) {
        std::cerr << "Bad timeout value. " 
            << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Something bad happened. "
            << e.what() << std::endl;
    }
    return 1;
}
