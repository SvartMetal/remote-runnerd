#ifndef PROCESS_RUNNER_H
#define PROCESS_RUNNER_H

#include <memory>
#include <vector>
#include <queue>
#include <string>
#include <utility>

#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "settings.h"

class ProcessRunner {
public: // constructors

    ProcessRunner(const std::vector<std::string>& config, boost::shared_mutex& config_mutex);

public: // methods
    
    void commit_data(const std::string& data);

    std::pair<bool, bool> attempt_launch();

    std::vector<char> get_execution_result();

    pid_t get_pid() { 
        boost::unique_lock<boost::mutex> lock(queue_mutex_);
        return pid_.load(); 
    }

private: // methods

    void exec_and_bind_stdout(const std::vector<std::string>& args);
    
    std::vector<std::string> tokenize_command(const std::string& cmd);

    bool search_cmd(const std::string& program);

    char** create_argv(const std::vector<std::string>& args);

private: // fields
    
    // Data buffer
    std::string data_;
    // Commands buffer
    std::queue<std::string> cmd_queue_;

    // Reference to config data & config sync stuff
    const std::vector<std::string>& config_;
    boost::shared_mutex& config_mutex_;

    // Command queue sync stuff
    boost::atomic<bool> is_running_;
    boost::mutex queue_mutex_;

    // Child process identifier, set to -1 in constructor
    boost::atomic<pid_t> pid_;

    // Stdout file struct
    FILE* stdout_ = 0; 

    enum {buffer_length = settings::process_buffer_length};
};

#endif // PROCESS_RUNNER_H