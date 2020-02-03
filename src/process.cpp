#include "process.hpp"

// paths to cgroups
std::string Process::freezer_path = "/sys/fs/cgroup/freezer/my_cgroup/";
std::string Process::cpuset_path = "/sys/fs/cgroup/cpuset/my_cgroup/";
//std::string cgrp = "my_cgroup";

Process::Process(std::string name,
        std::vector<std::string> argv,
        std::chrono::steady_clock::time_point start_time,
        std::chrono::nanoseconds budget,
        std::chrono::nanoseconds budget_jitter )
    : name(name),
    argv(argv),
    start_time(start_time),
    budget(budget),
    budget_jitter(budget_jitter),
    actual_budget(budget),
    cgroup(freezer_path + name + "/")
{
    timer_ptr->set<Process, &Process::timeout_cb>(this);
}

// testing
void Process::start_timer(std::chrono::nanoseconds timeout)
{
    timer_ptr->start(start_time + timeout);
}

bool Process::is_completed()
{
    return completed;
}

void Process::recompute_budget()
{
    std::chrono::nanoseconds rnd_val= budget_jitter * rand()/RAND_MAX;
    actual_budget = budget - budget_jitter/2 + rnd_val;
}

std::chrono::nanoseconds Process::get_actual_budget()
{
    return actual_budget;
}

void Process::exec()
{
    //TODO pipe

    // freeze cgroup
    cgroup.freeze();

    // create new process
    pid = fork();
    if( pid == -1 )
        kill_procs_and_exit("fork");

    // launch new process
    if( pid == 0 ){
        // CHILD PROCESS
        // add process to cgroup (echo PID > cgroup.procs)
        cgroup.add_process(getpid());

        // cast string to char*
        std::vector<char*> cstrings;
        cstrings.reserve( argv.size()+1 );

        for(size_t i = 0; i < argv.size(); ++i)
            cstrings.push_back(const_cast<char*>( argv[i].c_str() ));
        cstrings.push_back( (char*)NULL );

        if( execv( cstrings[0], &cstrings[0] ) == -1)
            kill_procs_and_exit("execv");
        // END CHILD PROCESS
    } else {
        // PARENT PROCESS
        spawned_processes.push_back(pid);
        // END PARENT PROCESS
    }
}

void Process::freeze()
{
    cgroup.freeze();
}

void Process::unfreeze()
{
    cgroup.unfreeze();
}

void Process::timeout_cb (ev::io &w, int revents)
{
    if (EV_ERROR & revents)
        err(1,"ev cb: got invalid event");

    std::cout << "timeout " << std::endl;
    // read to have empty fd
    uint64_t buf;
    int ret = read(w.fd, &buf, 10);
    if(ret != sizeof(uint64_t) )
        kill_procs_and_exit("read timerfd");
    w.stop();
}

void Process::set_cgroup_paths(std::string freezer, std::string cpuset)
{
    freezer_path = freezer;
    cpuset_path = cpuset;
}
