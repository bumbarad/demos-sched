#include "window.hpp"

Window::Window(ev::loop_ref loop_, std::chrono::milliseconds length_)
    : loop(loop_)
    , length(length_)
{}

Slice &Window::add_slice(Partition *sc, Partition *be, const cpu_set &cpus)
{
    auto sc_cb = [this](Slice &s, time_point t) { slice_sc_end_cb(s, t); };
    return slices.emplace_back(loop, sc_cb, sc, be, cpus);
}

void Window::start(time_point current_time)
{
    finished_sc_partitions = 0;
    for (auto &s : slices) {
        s.start_sc(current_time);
    }
}

void Window::stop(time_point current_time)
{
    for (auto &s : slices) {
        s.stop(current_time);
    }
}

void Window::slice_sc_end_cb([[maybe_unused]] Slice &slice, time_point current_time)
{
    // option 1) run BE immediately after SC
    // slice->start_be(current_time);

    // option 2) wait until all SC partitions finish
    finished_sc_partitions++;
    if (finished_sc_partitions == slices.size()) {
        for (auto &sp : slices) {
            sp.start_be(current_time);
        }
    }
}
