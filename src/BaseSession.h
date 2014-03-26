#ifndef BASE_SESSION_H
#define BASE_SESSION_H

/* This class needed for polymorphic dispatch when handling SIGCHLD */
struct BaseSession {
    virtual void handle_child_exit() = 0;

    virtual ~BaseSession() = default;
};

#endif