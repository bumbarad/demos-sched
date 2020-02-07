#ifndef MAJORFRAME_HPP
#define MAJORFRAME_HPP

#include "window.hpp"
#include "demossched.hpp"
#include "timerfd.hpp"
#include <list>
#include <ev++.h>

typedef std::list<Window> Windows;

class MajorFrame
{
public:
    MajorFrame(ev::loop_ref loop, Windows &windows );
    ~MajorFrame();
    void move_to_next_window();
    Window & get_current_window();
private:
    //std::chrono::nanoseconds length; // do we need this?
    ev::loop_ref loop;
    Windows &windows;
    Windows::iterator current;
    ev::timerfd timer;
    ev::sig sigint;
    void timeout_cb();
    void sigint_cb(ev::sig &w, int revents);
    void kill_all();
};

#endif // MAJORFRAME_HPP
