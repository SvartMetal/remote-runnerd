#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <map>
#include <memory>
#include <string>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "BaseSession.h"

typedef std::map<std::string, std::string> config_data_type;

typedef std::vector<char> buffer_type;

typedef std::map<pid_t, std::shared_ptr<BaseSession>> dispatcher_type;

/* This struct is a wrapper on synchronization stuff & shared data */
struct SyncData {
    const config_data_type& config;
    boost::shared_mutex& config_mutex;
    dispatcher_type& pid_to_session_map;
    boost::mutex& signal_mutex;

    SyncData(const config_data_type& config, 
        boost::shared_mutex& config_mutex,
        dispatcher_type& pid_to_session_map,
        boost::mutex& signal_mutex) 
        : config(config),
        config_mutex(config_mutex),
        pid_to_session_map(pid_to_session_map),
        signal_mutex(signal_mutex)
    {}
};

#endif