#include "functions.h"

std::vector<pid_t> spawned_processes;

// clean up everything and exit
void kill_procs_and_exit(std::string msg)
{
    std::cerr << msg <<std::endl;
    for(pid_t pid : spawned_processes){
        if( kill(pid, SIGKILL) == -1){
            warn("need to kill process %d manually", pid);
            // TODO clean cgroups
        }
    }
    exit(1);
}

void print_help()
{
    printf("help:\n"
            "-h\n"
            "\t print this message\n"
            "-g <NAME>\n"
            "\t name of cgroup with user access,\n"
            "\t need to create /sys/fs/cgroup/freezer/<NAME> and\n"
            "\t /sys/fs/cgroup/cpuset/<NAME> manually, then\n"
            "\t sudo chown -R <user> .../<NAME>\n"
            "\t if not set, \"my_cgroup\" is used\n"
            "TODO -c <FILE>\n"
            "\t path to configuration file\n");
}
