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
    
    void tcp_accept();
    void configure_tcp_endpoint();

    #ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    void local_accept();
    void configure_local_endpoint();
    #endif

    void handle_update_config();
    void handle_stop();

private: // fields

    size_t thread_pool_size_;
    size_t timeout_;

    // Asio stuff 
    // Must be initialized before passing to other members
    boost::asio::io_service io_service_;

    // Signal sets for handling signals
    boost::asio::signal_set quit_signals_;

    boost::asio::signal_set update_config_signal_;

    // Socket acceptors & endpoints
    boost::asio::ip::tcp::acceptor tcp_acceptor_;
    boost::asio::ip::tcp::endpoint tcp_endpoint_;

    #ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    boost::asio::local::stream_protocol::acceptor local_acceptor_; 
    boost::asio::local::stream_protocol::endpoint local_endpoint_;
    #endif

    // Sync & config stuff
    ConfigParser config_parser_;
    config_data_type config_;
    boost::shared_mutex config_mutex_;

};

#endif // SERVER_H