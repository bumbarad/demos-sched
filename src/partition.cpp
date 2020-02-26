#include "partition.hpp"
#include <algorithm>
#include <string.h>

using namespace std::placeholders;
using namespace std;

Partition::Partition(Cgroup &freezer_parent,
                     Cgroup &cpuset_parent,
                     Cgroup &events_parent,
                     std::string name)
    : cgf(freezer_parent, name)
    , cgc(cpuset_parent, name)
    , cge(events_parent, name)
    , current(nullptr)
    , name(name)
{
#ifdef VERBOSE
    std::cerr << __PRETTY_FUNCTION__ << " " << name << std::endl;
#endif
    freeze();
}

Process &Partition::get_current_proc()
{
    return *current;
}

void Partition::freeze()
{
    for (Process &p : processes)
        p.freeze();
}

void Partition::unfreeze()
{
    for (Process &p : processes)
        p.unfreeze();
}

void Partition::add_process(ev::loop_ref loop,
                            string argv,
                            chrono::nanoseconds budget,
                            chrono::nanoseconds budget_jitter)
{
    // std::cerr<<__PRETTY_FUNCTION__<<" "<<name<<std::endl;

    // get process name, after the last / in argv[0]
    //    char *token, *cmd_name, *saveptr;
    //    for ( char* cmd = argv[0];; cmd = nullptr) {
    //        token = strtok_r(cmd,"/",&saveptr);
    //        if( token == nullptr )
    //            break;
    //        cmd_name = token;
    //    }

    // cerr<< cmd_name <<endl;

    processes.emplace_back(
      loop, to_string(proc_count) + "todo", *this, argv, budget, budget_jitter);
    proc_count++;
    current = --processes.end();
    current->exec();
    cgc.add_process(current->get_pid());
    empty = false;
}

void Partition::set_cpus(string cpus)
{
    cgc.set_cpus(cpus);
}

void Partition::move_to_first_proc()
{
    current = processes.begin();
}

// return false if there is none
bool Partition::move_to_next_unfinished_proc()
{
    for (size_t i = 0; i < processes.size(); i++) {
        move_to_next_proc();
        if (!current->is_completed())
            return true;
    }
    completed = true;
    return false;
}

bool Partition::is_completed()
{
    return completed;
}

void Partition::clear_completed_flag()
{
    completed = false;
    for (Process &p : processes)
        p.mark_uncompleted();
}

bool Partition::is_empty()
{
    return empty;
}

void Partition::kill_all()
{
    for (Process &p : processes)
        p.kill();
}

void Partition::bind_empty_cb(std::function<void()> new_empty_cb)
{
    empty_cbs.push_back(new_empty_cb);
}

string Partition::get_name()
{
    return name;
}

// cyclic queue
void Partition::move_to_next_proc()
{
    if (++current == processes.end())
        current = processes.begin();
}

void Partition::proc_exit_cb(Process &proc)
{
#ifdef VERBOSE
    cerr << __PRETTY_FUNCTION__ << " partition: " << name << " pid: " << proc.get_pid() << endl;
#endif

    // check if there is no running processes in this partition
    for( auto &p : processes) {
        if( p.is_running() )
            return;
    }

    empty = true;
    // notify all slices which owns this partition that there is no running process
    for (auto &cb : empty_cbs)
        cb();

}
