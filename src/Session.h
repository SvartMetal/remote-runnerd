#ifndef SESSION_H
#define SESSION_H

#include <memory>

#include <boost/asio.hpp>

#include "settings.h"
#include "types.h"
#include "ProcessRunner.h"

template <typename Socket>
class Session : public std::enable_shared_from_this<Session<Socket>> {
public: // constructors

    Session(boost::asio::io_service& io_service,
        size_t timeout, 
        const config_data_type& config, 
        boost::shared_mutex& config_mutex);

    /* Noncopyable */
    Session(const Session&) = delete;
    Session& operator = (const Session&) = delete;

public: // methods
    
    /*
        Starts async read operation.
    */
    void start();

    /*
        Returns session socket reference.
        This reference needed for accepting session.
    */
    Socket& socket();

private: // methods

    void do_read();

    void do_write(const std::string& data);
    void do_write(const buffer_type& buffer);

    void try_launch_process();

    void handle_child_exit();


private: // fields
    
    // Strand needed for synchronization
    boost::asio::io_service::strand strand_;

    Socket socket_;

    ProcessRunner process_runner_;

    // Signal set for handling SIGCHLD
    boost::asio::signal_set child_exit_signal_;

    // Timer for killing child
    boost::asio::deadline_timer timer_;
    // Child timeout
    boost::posix_time::seconds timeout_;

    enum {buffer_length = settings::session_buffer_length};
    char data_[buffer_length];
};

template<class T>
Session<T>::Session(boost::asio::io_service& io_service, 
    size_t timeout,
    const config_data_type& config,
    boost::shared_mutex& config_mutex)

    : strand_(io_service), 
    socket_(io_service),
    process_runner_(config, config_mutex),
    child_exit_signal_(io_service),
    timer_(io_service),
    timeout_(timeout)
{
    child_exit_signal_.add(SIGCHLD);
}

template<class T>
void Session<T>::start() {
    // Wrapping session in shared pointer to prevent 
    // memory invalidation when handler will be executed
    auto self(this->shared_from_this());
    child_exit_signal_.async_wait(strand_.wrap(
        boost::bind(&Session::handle_child_exit, self)));

    // Start reading data asynchronously!
    do_read();
}

template<class Socket>
Socket& Session<Socket>::socket() {
    return socket_;
}

template<class T>
void Session<T>::do_read() {
    // Wrapping session in shared pointer to prevent 
    // memory invalidation when handler will be executed 
    auto self(this->shared_from_this());

    socket_.async_read_some(boost::asio::buffer(data_, buffer_length),
        strand_.wrap([this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                process_runner_.commit_data(std::string(data_, length));
                try_launch_process();
                do_read();
            }
        }));
}

template<class T>
void Session<T>::try_launch_process() {
    auto result = process_runner_.attempt_launch();
    auto task_id = result.task_id;
    
    if (result.launched) {
        auto self(this->shared_from_this());
        timer_.expires_from_now(timeout_);

        timer_.async_wait(strand_.wrap([this, self, task_id](boost::system::error_code ec) {
            if (ec != boost::asio::error::operation_aborted) {
                // Checking if timer was not cancelled
                process_runner_.kill_task(task_id);
            }
        }));
    } else if (result.attempted) {
        // Attempt to launch process failed
        std::string error_msg = "Invalid command\n";
        do_write(error_msg);
    }
}

template<class T>
void Session<T>::do_write(const std::string& data) {
    do_write(buffer_type(data.c_str(), data.c_str() + data.length() + 1));
}

template<class T>
void Session<T>::do_write(const buffer_type& buffer) {
    if (buffer.empty()) {
        // There is no data to write
        return;
    }
    auto self(this->shared_from_this());
    auto data_ptr = std::make_shared<buffer_type>(buffer);

    boost::asio::async_write(socket_, boost::asio::buffer(&(*data_ptr)[0], data_ptr->size()),
        strand_.wrap([this, self, data_ptr](boost::system::error_code ec, size_t) {}));
}

template<class T>
void Session<T>::handle_child_exit() {
    // SIGCHLD received
    // Need to cancel timer task
    timer_.cancel();
    // Creating new async task before child can be executed again
    auto self(this->shared_from_this());
    child_exit_signal_.async_wait(
        strand_.wrap(boost::bind(&Session::handle_child_exit, self)));

    buffer_type stdout;
    buffer_type stderr;
    auto status = process_runner_.write_execution_result(stdout, stderr); 
    if (!status) {
        // All is OK, writing stdout to client
        do_write("Execution is successful\n");
    } else {
        std::string error_msg = "Execution error. Exit code: ";
        error_msg += std::to_string(status);
        error_msg += "\n";
        do_write(error_msg);
    }
    do_write("*** STDOUT ***\n");
    do_write(stdout);
    do_write("*** STDERR ***\n");
    do_write(stderr);
    // Go on launching queued commands
    try_launch_process();
}

#endif // SESSION_H