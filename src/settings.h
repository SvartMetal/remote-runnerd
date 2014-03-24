#ifndef SETTINGS_H
#define SETTINGS_H

struct settings {
    static const constexpr char* config_file_name = "/etc/remote-runnerd.conf";
    static const constexpr char* local_socket_address = "/tmp/simple-telnetd";
    static const size_t server_thread_pool_size = 5;
    static const size_t port = 12345;
    static const size_t session_buffer_length = 1024;
    static const size_t process_buffer_length = 1024;
};

#endif // SETTINGS_H

