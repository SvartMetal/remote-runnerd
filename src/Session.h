#ifndef SESSION_H
#define SESSION_H

#include <memory>

#include <boost/asio.hpp>

#include "settings.h"
#include "ProcessRunner.h"

class Session : public std::enable_shared_from_this<Session> {
public: // constructors

    Session(boost::asio::io_service& io_service, 
        const std::vector<std::string>& config, 
        boost::shared_mutex& config_mutex);

    /* Noncopyable */
    Session(const Session&) = delete;
    Session& operator = (const Session&) = delete;

public: // methods
    
    void start();

    boost::asio::ip::tcp::socket& socket();

private: // methods

    void do_read();

    void do_write(const std::string& data);
    void do_write(const std::vector<char>& buffer);

    void try_launch_process();

    void handle_child_exit();


private: // fields
    
    // Strand needed for synchronization
    boost::asio::io_service::strand strand_;

    boost::asio::ip::tcp::socket socket_;

    ProcessRunner process_runner_;

    // Signal set for handling SIGCHLD
    boost::asio::signal_set child_exit_signal_;

    enum {buffer_length = settings::session_buffer_length};
    char data_[buffer_length];
};

#endif // SESSION_H