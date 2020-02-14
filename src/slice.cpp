#include "slice.hpp"

Slice::Slice(ev::loop_ref loop, std::chrono::steady_clock::time_point start_time,
             Partition &sc, Partition &be, std::string cpus)
    : sc(sc)
    , be(be)
    , cpus(cpus)
    , timeout( start_time )
    , timer(loop)
{
    timer.set(std::bind(&Slice::timeout_cb, this));
    // timer.start(start_time + timeout);
}

void Slice::move_proc_and_start_timer( Partition &p )
{
    p.move_to_next_unfinished_proc();
    timeout += p.get_current_proc().get_actual_budget();
    timer.start(timeout);
}

void Slice::start()
{
    sc.clear_completed_flag();
    be.clear_completed_flag();

//    sc.set_cpus(cpus);
//    be.set_cpus(cpus);

    if( !sc.is_empty() ){
        sc.move_to_first_proc();
        current_proc = &sc.get_current_proc();
        current_proc->unfreeze();
        timeout += current_proc->get_actual_budget();
        timer.start(timeout);
    } else if ( !be.is_empty() ) {
        be.move_to_first_proc();
        current_proc = &be.get_current_proc();
        current_proc->unfreeze();
        timeout += current_proc->get_actual_budget();
        timer.start(timeout);
    }
}

void Slice::stop()
{
    if(current_proc)
        current_proc->freeze();
    timer.stop();
}

void Slice::update_timeout(std::chrono::steady_clock::time_point actual_time)
{
    timeout = actual_time;
}

void Slice::timeout_cb()
{
#ifdef VERBOSE
    std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

    current_proc->freeze();
    current_proc->mark_completed();

    //std::cerr<< sc.is_done() <<std::endl;

    if( !sc.is_empty() && !sc.is_completed() && sc.move_to_next_unfinished_proc() ){
        current_proc = &sc.get_current_proc();
        current_proc->unfreeze();
        timeout += current_proc->get_actual_budget();
        timer.start(timeout);
        return;
    } else if( !be.is_empty() && !be.is_completed() && be.move_to_next_unfinished_proc() ) {
        current_proc = &be.get_current_proc();
        current_proc->unfreeze();
        timeout += current_proc->get_actual_budget();
        timer.start(timeout);
        return;
    }

    timer.stop();
}
