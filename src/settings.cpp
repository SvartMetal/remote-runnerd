#include "settings.h"

const char* settings::config_file_name = "/etc/remote-runnerd.conf";

const char* settings::local_socket_address = "/tmp/simple-telnetd";

const size_t settings::server_thread_pool_size = 5;

const size_t settings::port = 12345;