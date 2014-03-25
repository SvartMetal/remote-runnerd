#include <sys/wait.h>
#include <csignal>
#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include "ProcessRunner.h"

ProcessRunner::ProcessRunner(const config_data_type& config, boost::shared_mutex& config_mutex)
    : config_(config),
    config_mutex_(config_mutex),
    is_running_(false), 
    pid_(-1),
    task_id_(0),
    stdout_(nullptr),
    stderr_(nullptr)
{}

void ProcessRunner::commit_data(const std::string& data) {
    data_ += data;
    size_t pos = data_.find_first_of('\n');
    if (pos != std::string::npos) {
        // Extract command from buffer
        auto cmd = data_.substr(0, pos);

        // Command can't consist only of whitespace symbols
        boost::algorithm::trim(cmd);

        // Erasing current command from data buffer
        data_.erase(0, pos + 1);

        if (cmd.empty()) {
            // Ignoring empty commands
            return;
        }
        
        boost::unique_lock<boost::mutex> lock(queue_mutex_);
        cmd_queue_.push(cmd);
    }
}

std::vector<std::string> ProcessRunner::tokenize_cmd(const std::string& cmd) const {
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

// pair.first = true if command was found
std::pair<bool, std::string> ProcessRunner::search_cmd(const std::string& cmd) {
    // Reader lock
    boost::shared_lock<boost::shared_mutex> lock(config_mutex_);

    // Searching for match
    auto found = config_.find(cmd) != config_.end();
    return found ? std::make_pair(true, config_.at(cmd)) : std::make_pair(false, "");
}

ProcessRunner::AttemptStatus ProcessRunner::attempt_launch() {
    boost::unique_lock<boost::mutex> queue_lock(queue_mutex_);
    boost::unique_lock<boost::mutex> child_lock(child_mutex_);
    if (is_running_ || cmd_queue_.empty()) {
        // Child is already running or nothing to execute
        return AttemptStatus(false, false, task_id_);
    }
    
    auto cmd = cmd_queue_.front();
    cmd_queue_.pop();
    queue_lock.unlock();
    // Checking command
    auto args = tokenize_cmd(cmd);        
    if (args.empty()) {
        // Command is invalid
        return AttemptStatus(true, false, task_id_);
    }
    auto search_result = search_cmd(args[0]);
    if (!search_result.first) {
        return AttemptStatus(true, false, task_id_);
    }
    auto program = search_result.second;
    args[0] = program;
    
    // Create pipe, fork, exec and acquire child's stdout file struct pointer 
    auto pid = exec_and_bind_streams(args);

    if (pid == -1) {
        // Launch failed
        return AttemptStatus(true, false, task_id_);
    }

    // Launch is successful
    // Creating execution context
    is_running_ = true;
    pid_ = pid;
    return AttemptStatus(true, true, task_id_);
}

void ProcessRunner::set_parent_descriptors(int pipe_stdout[2], int pipe_stderr[2]) {
    close(pipe_stdout[1]);
    close(pipe_stderr[1]);
    stdout_ = fdopen(pipe_stdout[0], "r");
    stderr_ = fdopen(pipe_stderr[0], "r");
}

void ProcessRunner::set_child_descriptors(int pipe_stdout[2], int pipe_stderr[2]) {
    close(pipe_stdout[0]);
    close(pipe_stderr[0]);
    dup2(pipe_stdout[1], STDOUT_FILENO);
    dup2(pipe_stderr[1], STDERR_FILENO);
    close(pipe_stdout[1]);
    close(pipe_stderr[1]);
}

// Must be synchronized
pid_t ProcessRunner::exec_and_bind_streams(const std::vector<std::string>& args) {
    // Creating pipe
    const char* program = args[0].c_str();
    int pipe_stdout[2];
    int pipe_stderr[2];
    if (pipe(pipe_stdout) || pipe(pipe_stderr)) {
        // Pipe error occured
        return -1;
    }

    auto pid = fork();

    if (pid < 0) {
        // Fork error occured
        return -1;
    }

    if (pid != 0) {
        set_parent_descriptors(pipe_stdout, pipe_stderr);
        return pid;
    } else {
        // We are in the child
        set_child_descriptors(pipe_stdout, pipe_stderr);
        // Copying argv for new process executing
        char** argv = create_argv(args);

        execv(program, argv);
        perror("child");
        exit(1);
    }
    return -1;
}

char** ProcessRunner::create_argv(const std::vector<std::string>& args) const {
    char** argv = new char*[args.size() + 1];
    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = new char[args[i].length() + 1];
        strcpy(argv[i], args[i].c_str());
    }
    // Arguments must be guarded by NULL
    argv[args.size()] = nullptr;
    return argv;
}

int ProcessRunner::write_execution_result(buffer_type& stdout_buf, buffer_type& stderr_buf) {
    boost::unique_lock<boost::mutex> lock(child_mutex_);
    if (pid_ == -1) return 1;

    int status;
    // Acquiring child exit code 
    waitpid(pid_, &status, 0);

    // Copying file struct pointers
    FILE* stdout = stdout_;
    FILE* stderr = stderr_;
    // Clearing context for the next launch
    clear_context();     
    // Ready for new task!
    ++task_id_;
    lock.unlock();

    if (stdout && stderr) {
        // Now we can read child's stdout and stderr
        read_file_to_buf(stdout_buf, stdout);
        read_file_to_buf(stderr_buf, stderr);
        
        fclose(stdout);
        fclose(stderr);
    }
    return status;
}

// Must be synchronized
void ProcessRunner::clear_context() {
    is_running_ = false;
    stdout_ = 0;
    stderr_ = 0;
    pid_ = -1;
}

void ProcessRunner::read_file_to_buf(buffer_type& result, FILE* file) {
    while (!feof(file)) {
        int bytes_read;
        if ((bytes_read = fread(buf_, sizeof(char), buffer_length, file)) != 0) {
            for (size_t i = 0; i < bytes_read; ++i) {
                result.push_back(buf_[i]);
            }
        }
    }
}

void ProcessRunner::kill_task(size_t id) {
    boost::unique_lock<boost::mutex> lock(child_mutex_);
    if (id == task_id_ && pid_ != -1) {
        kill(pid_.load(), SIGKILL);
    }

}

