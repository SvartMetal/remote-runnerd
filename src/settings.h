#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdlib>

struct settings {
    static const char* config_file_name;
    static const char* local_socket_address;
    static const size_t server_thread_pool_size;
    static const size_t port;
    static const constexpr size_t session_buffer_length = 1024;
    static const constexpr size_t process_buffer_length = 1024;
};

#endif // SETTINGS_H

