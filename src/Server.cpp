#include <stdexcept>

#include "Server.h"

using boost::asio::ip::tcp;

Server::Server(short port,
    size_t thread_pool_size,
    size_t timeout)

    : thread_pool_size_(thread_pool_size),
    timeout_(timeout),
    quit_signals_(io_service_),
    update_config_signal_(io_service_),
    acceptor_(io_service_),
    config_parser_(settings::config_file_name),
    config_(config_parser_.parse_config())
{
    // Setting signals 
    quit_signals_.add(SIGINT);
    quit_signals_.add(SIGTERM);

    #ifdef BOOST_SIGQUIT
    quit_signals_.add(SIGQUIT);
    #endif

    // Signal for config updating
    update_config_signal_.add(SIGHUP);

    // Configuring server endpoint
    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(tcp::v4(), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint, ec);
    acceptor_.listen();

    if (ec) {
        throw std::logic_error("Port is already used. ");
    }

    // Setting handlers for signals 
    quit_signals_.async_wait(boost::bind(&Server::handle_stop, this));
    update_config_signal_.async_wait(boost::bind(&Server::handle_update_config, this));

    do_accept();
}

void Server::run() {
    // Our thread pool
    std::vector<std::shared_ptr<boost::thread>> threads;

    for (size_t i = 0; i < thread_pool_size_; ++i) {
        std::shared_ptr<boost::thread> thread_ptr(new boost::thread(
            boost::bind(&boost::asio::io_service::run, &io_service_)));
        threads.push_back(thread_ptr);
    }

    for (auto& thread : threads) {
        thread->join();
    }
}

void Server::do_accept() {
    auto session = std::make_shared<Session>(io_service_, config_, config_mutex_);
    acceptor_.async_accept(session->socket(), 
        [this, session](boost::system::error_code ec) {
            if (!ec) {
                session->start();
            }
            // Go on accepting new sessions
            do_accept();
        });
}

void Server::handle_update_config() {
    update_config_signal_.async_wait(boost::bind(&Server::handle_update_config, this));
    // Writer lock
    boost::unique_lock<boost::shared_mutex> lock(config_mutex_);
    config_ = config_parser_.parse_config();
}

void Server::handle_stop() {
    io_service_.stop();
}

