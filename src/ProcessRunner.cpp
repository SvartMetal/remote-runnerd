#include <cstdlib>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>

#include "ProcessRunner.h"

ProcessRunner::ProcessRunner(const std::vector<std::string>& config, boost::shared_mutex& config_mutex)
    : config_(config),
    config_mutex_(config_mutex),
    is_running_(false), 
    pid_(-1)
{}

void ProcessRunner::commit_data(const std::string& data) {
    data_ += data;
    size_t pos = data_.find_first_of('\n');
    if (pos != std::string::npos) {
        // Extract command from buffer
        auto cmd = data_.substr(0, pos);
        // Command can't consist only of whitespace symbols
        boost::algorithm::trim(cmd);

        boost::unique_lock<boost::mutex> lock(queue_mutex_);
        cmd_queue_.push(cmd);

        // Clearing data buffer for new commands
        data_.erase(0, pos + 1);
    }
}

std::vector<std::string> ProcessRunner::tokenize_command(const std::string& cmd) {
    boost::char_separator<char> sep(" \t");
    boost::tokenizer<boost::char_separator<char>> tokenizer(cmd, sep);

    std::vector<std::string> args;
    auto end = tokenizer.end();

    if (tokenizer.begin() == end) {
        args.push_back("");
    } else {
        for (auto it = tokenizer.begin(); it != end; ++it) {
            args.push_back(*it);
        }
    }
    return args;
}

bool ProcessRunner::search_cmd(const std::string& program) {
    // Reader lock
    boost::shared_lock<boost::shared_mutex> lock(config_mutex_);

    // Searching for match
    for (const auto& elem : config_) {
        if (elem == program) {
            return true;
        }
    }

    return false;
}

std::pair<bool, bool> ProcessRunner::attempt_launch() {
    boost::unique_lock<boost::mutex> lock(queue_mutex_);
    if (is_running_ || cmd_queue_.empty()) {
        // Child is already running or nothing to execute
        return std::make_pair(false, false);
    }
    
    auto cmd = cmd_queue_.front();
    cmd_queue_.pop();
    // Checking command
    auto args = tokenize_command(cmd);        
    if (args.empty() || !search_cmd(args[0])) {
        // Command is invalid
        pid_ = -1;
        return std::make_pair(true, false);
    }
    std::cout << cmd << std::endl;
    
    // Create pipe, fork, exec and acquire child's stdout file struct pointer 
    exec_and_bind_stdout(args);

    if (pid_ == -1) {
        // Launch failed
        return std::make_pair(true, false);
    }

    // Launch is successful
    // Creating execution context
    is_running_ = true;
    return std::make_pair(true, true);
}

// Must be syncronized
void ProcessRunner::exec_and_bind_stdout(const std::vector<std::string>& args) {
    // Creating pipe
    const char* program = args[0].c_str();
    int pipe_stdout[2];
    if (pipe(pipe_stdout)) {
        pid_ = -1;
        return;
    }

    pid_ = fork();

    if (pid_ < 0) {
        // Fork error occured
        pid_ = -1;
        return;
    }

    if (pid_ != 0) {
        // We are in the parent now 
        close(pipe_stdout[1]);
        stdout_ = fdopen(pipe_stdout[0], "r");
    } else {
        // We are in the child
        close(pipe_stdout[0]);
        dup2(pipe_stdout[1], STDOUT_FILENO);
        close(pipe_stdout[1]);

        // Copying argv for new process executing
        char** argv = create_argv(args);

        execv(program, argv);
        perror("child");
        exit(1);
    }
}

std::vector<char> ProcessRunner::get_execution_result() {
    boost::unique_lock<boost::mutex> lock(queue_mutex_);
    // Copying file struct pointer
    FILE* stdout = stdout_;
    // Clearing context for the next launch
    is_running_ = false;
    stdout_ = 0;
    pid_ = -1;
    lock.unlock();

    if (stdout) {
        std::vector<char> result;
        char buffer[buffer_length];

        // Now we can read child's stdout
        while (!feof(stdout)) {
            int bytes_read;
            if ((bytes_read = fread(buffer, sizeof(char), buffer_length, stdout)) != 0) {
                for (size_t i = 0; i < bytes_read; ++i) {
                    result.push_back(buffer[i]);
                }
            }
        }

        fclose(stdout);
        return result;
    }

    return std::vector<char>();
}

char** ProcessRunner::create_argv(const std::vector<std::string>& args) {
    char** argv = new char*[args.size() + 1];

    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = new char[args[i].length() + 1];
        strcpy(argv[i], args[i].c_str());
    }
    // Arguments must be guarded by NULL
    argv[args.size()] = nullptr;
    return argv;
}
