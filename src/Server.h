#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <string>
#include <memory>

#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include "Session.h"
#include "ConfigParser.h"

class Server {
public: // constructors

    Server(short port,
        size_t thread_pool_size,
        size_t timeout);

    /* Noncopyable */
    Server(const Server&) = delete;
    Server& operator = (const Server&) = delete;

public: // methods
    
    void run();

private: // methods
    
    void do_accept();
    void handle_update_config();
    void handle_stop();

private: // fields

    size_t thread_pool_size_;
    size_t timeout_;

    // Asio stuff 
    boost::asio::io_service io_service_;

    boost::asio::signal_set quit_signals_;

    boost::asio::signal_set update_config_signal_;

    boost::asio::ip::tcp::acceptor acceptor_;

    // Sync & config stuff
    ConfigParser config_parser_;
    std::vector<std::string> config_;
    boost::shared_mutex config_mutex_;
};

#endif // SERVER_H