#include <cstdlib>

#include "Session.h"

#include <iostream>

Session::Session(boost::asio::io_service& io_service, 
    const std::vector<std::string>& config,
    boost::shared_mutex& config_mutex)
    : strand_(io_service), 
    socket_(io_service),
    process_runner_(config, config_mutex),
    child_exit_signal_(io_service)
{
    child_exit_signal_.add(SIGCHLD);
}

void Session::start() {
    // Wrapping session in shared pointer to prevent 
    // memory invalidation when handler will be executed
    auto self(shared_from_this());
    child_exit_signal_.async_wait(boost::bind(&Session::handle_child_exit, self));

    // Start reading data asynchronously!
    do_read();
}

boost::asio::ip::tcp::socket& Session::socket() {
    return socket_;
}

void Session::do_read() {
    // Wrapping session in shared pointer to prevent 
    // memory invalidation when handler will be executed 
    auto self(shared_from_this());

    socket_.async_read_some(boost::asio::buffer(data_, buffer_length),
        strand_.wrap([this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                process_runner_.commit_data(std::string(data_, length));
                try_launch_process();
                std::cout << "Read " << length << " bytes. " << std::endl;
                do_read();
            }
        }));

}

void Session::try_launch_process() {
    auto attempted = process_runner_.attempt_launch();
    auto pid = process_runner_.get_pid();
    
    if (attempted && pid == -1) {
        // Attempt to launch process failed
        std::string error_msg = "Process can't be run. Invalid command\n";
        do_write(error_msg);
    }
}

void Session::do_write(const std::string& data) {
    do_write(std::vector<char>(data.c_str(), data.c_str() + data.length() + 1));
}

void Session::do_write(const std::vector<char>& buffer) {
    if (buffer.empty()) {
        // There is no data to write
        return;
    }
    auto self(shared_from_this());
    auto data_ptr = std::make_shared<std::vector<char>>(buffer);

    boost::asio::async_write(socket_, boost::asio::buffer(&(*data_ptr)[0], data_ptr->size()),
        strand_.wrap([this, self, data_ptr](boost::system::error_code ec, size_t) {}));
}

void Session::handle_child_exit() {
    // Creating new async task before child can be executed again
    auto self(shared_from_this());
    child_exit_signal_.async_wait(boost::bind(&Session::handle_child_exit, self));

    // SIGCHLD received
    auto pid = process_runner_.get_pid();    
    if (pid != -1) {
        int status;
        // Acquiring child exit code
        waitpid(pid, &status, 0);
        auto stdout = process_runner_.get_execution_result(); 
        if (!status) {
            // All is OK, writing stdout to client
            do_write(stdout);
        } else {
            std::string error_msg = "Process exited with exit code: ";
            error_msg += std::to_string(status);
            error_msg += "\n";
            do_write(error_msg);
        }
    } 

    // Go on launching queued commands
    try_launch_process();
}
