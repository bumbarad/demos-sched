#include "classes.hpp"

Partition::Partition(Processes &&processes )
    : processes(std::move(processes))
    , current( this->processes.begin() )
{ }

Process & Partition::get_current_proc()
{
    return *current;
}

// cyclic queue?
void Partition::move_to_next_proc()
{
    if( ++current == processes.end() )
        current = processes.begin();
}
