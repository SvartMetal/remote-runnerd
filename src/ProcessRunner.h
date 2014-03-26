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
#include "types.h"

class ProcessRunner {
public: // constructors

    ProcessRunner(const SyncData& sync_data);

public: // structs

    /* Needed for wrapping attempt_launch method return value */ 
    struct AttemptStatus {
        bool attempted;
        bool launched;
        size_t task_id;

        AttemptStatus(bool attempted, bool launched, size_t task_id)
            : attempted(attempted), launched(launched), task_id(task_id)
        {}
    };

public: // methods
    
    /*
        Appends data to data buffer. 
        After that performs parsing and enqueues command to
        command queue (if command was parsed).
    */
    void commit_data(const std::string& data);

    /* 
        Returns AttemptResult struct in which: 
        'attempted' is true if child launch attempted,
        'launched' is true if child launched successfully
        'task_id' - launched task id.
    */
    AttemptStatus attempt_launch();

    /*
        Writes stdout and stderr to corresponding arguments. 
        Returns child exit code.
    */
    int write_execution_result(buffer_type& stdout_buf, buffer_type& stderr_buf);

    /*
        Kills child task if 'id' equals to current task id.
    */
    void kill_task(size_t id);

    /*
        Need this method because of 'chicken & egg' problem.
    */
    void initialize_with_session(const std::shared_ptr<BaseSession>& session);

private: // methods

    // Command parsing utils
    std::vector<std::string> tokenize_cmd(const std::string& cmd) const;
    std::pair<bool, std::string> search_cmd(const std::string& cmd);

    // Child execution utils
    void set_parent_descriptors(int pipe_stdout[2], int pipe_stderr[2]);
    void set_child_descriptors(int pipe_stdout[2], int pipe_stderr[2]);
    pid_t exec_and_bind_streams(const std::vector<std::string>& args);
    char** create_argv(const std::vector<std::string>& args) const;

    void clear_context();
    void read_file_to_buf(buffer_type& result, FILE* file);


private: // fields
    
    // Data buffer
    std::string data_;
    // Commands buffer
    std::queue<std::string> cmd_queue_;

    // Command queue sync stuff
    boost::mutex queue_mutex_;

    // Reference to config data 
    const config_data_type& config_;
    // Mutex for reader lock
    boost::shared_mutex& config_mutex_;

    // SIGCHLD Dispatching stuff
    dispatcher_type& pid_to_session_map_;
    boost::mutex& signal_mutex_;

    // Current session
    std::shared_ptr<BaseSession> session_;
    
    /* Child sync stuff */
    boost::atomic<bool> is_running_;
    // Child process identifier, set to -1 in constructor
    boost::atomic<pid_t> pid_;
    // Current child task id, set to 0 in constructor
    boost::atomic<size_t> task_id_;
    // Mutex for child shared data
    boost::mutex child_mutex_;
    // Standard output file struct pointer wrapper
    boost::atomic<FILE*> stdout_; 
    // Standard error file struct pointer wrapper
    boost::atomic<FILE*> stderr_;

    enum {buffer_length = settings::process_buffer_length};
    char buf_[buffer_length];
};

#endif // PROCESS_RUNNER_H