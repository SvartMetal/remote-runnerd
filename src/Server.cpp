#include <stdexcept>

#include "Server.h"

using boost::asio::ip::tcp;
using boost::asio::local::stream_protocol;

Server::Server(short port,
    size_t thread_pool_size,
    size_t timeout)

    : thread_pool_size_(thread_pool_size),
    timeout_(timeout),
    quit_signals_(io_service_),
    update_config_signal_(io_service_),
    config_parser_(settings::config_file_name),
    config_(config_parser_.parse_config()),
    tcp_acceptor_(io_service_),
    tcp_endpoint_(tcp::endpoint(tcp::v4(), port)),

    #ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    local_acceptor_(io_service_),
    local_endpoint_(settings::local_socket_address)
    #endif

{
    if (config_.empty()) { throw std::logic_error("Config is invalid. "); }
    // Setting quit signals
    quit_signals_.add(SIGINT);
    quit_signals_.add(SIGTERM);

    #ifdef BOOST_SIGQUIT
    quit_signals_.add(SIGQUIT);
    #endif
    // Signal for config updating
    update_config_signal_.add(SIGHUP);
    // Setting handlers for signals
    quit_signals_.async_wait(boost::bind(&Server::handle_stop, this));
    update_config_signal_.async_wait(boost::bind(&Server::handle_update_config, this));

    // Configure endpoints and starting listening for connections
    configure_tcp_endpoint();
    configure_local_endpoint();
}

void Server::configure_tcp_endpoint() {
    // Trying to bind tcp endpoint
    boost::system::error_code ec;
    tcp_acceptor_.open(tcp_endpoint_.protocol());
    tcp_acceptor_.set_option(tcp::acceptor::reuse_address(true));
    tcp_acceptor_.bind(tcp_endpoint_, ec);

    if (ec) { throw std::logic_error("Port is already used. "); }

    tcp_acceptor_.listen();
    tcp_accept();
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
void Server::configure_local_endpoint() {
    // Need to unbind address
    ::unlink(settings::local_socket_address);

    boost::system::error_code ec;
    local_acceptor_.open(local_endpoint_.protocol());
    local_acceptor_.set_option(stream_protocol::acceptor::reuse_address(true));
    local_acceptor_.bind(local_endpoint_, ec);

    if (ec) { throw std::logic_error("Local address is already used. "); }

    local_acceptor_.listen();
    local_accept();
}
#endif

void Server::run() {
    // Initialize worker threads pool
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

void Server::process_child_exit(pid_t pid) {
    boost::unique_lock<boost::mutex> lock(signal_mutex_);

    auto it = pid_to_session_map_.find(pid);
    if (it != pid_to_session_map_.end()) {
        auto session = pid_to_session_map_.at(pid);
        // Need to erase element from map before handling
        // child exit, because someone can obtain same pid

        lock.unlock();

        pid_to_session_map_.erase(it->first);
        session->handle_child_exit();
    }
}

void Server::tcp_accept() {
    SyncData sync_data(config_, config_mutex_, pid_to_session_map_, signal_mutex_);
    // Create new session to accept
    auto session = std::make_shared<Session<tcp::socket>>(io_service_, timeout_, sync_data);

    tcp_acceptor_.async_accept(session->socket(),
        [this, session](boost::system::error_code ec) {
            if (!ec) {
                session->start();
            }
            // Go on accepting new sessions
            tcp_accept();
        });
}

#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
void Server::local_accept() {
    SyncData sync_data(config_, config_mutex_, pid_to_session_map_, signal_mutex_);
    auto session = std::make_shared<Session<stream_protocol::socket>>(io_service_, timeout_, sync_data);

    local_acceptor_.async_accept(session->socket(),
        [this, session](boost::system::error_code ec) {
            if (!ec) {
                session->start();
            }
            // Go on accepting new sessions
            local_accept();
        });
}
#endif

void Server::handle_update_config() {
    update_config_signal_.async_wait(boost::bind(&Server::handle_update_config, this));
    // Writer lock
    boost::unique_lock<boost::shared_mutex> lock(config_mutex_);
    config_ = config_parser_.parse_config();
}

void Server::handle_stop() {
    io_service_.stop();
}
