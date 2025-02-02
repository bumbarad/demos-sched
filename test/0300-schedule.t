#!/usr/bin/env bash
. testlib
plan_tests 18


if ! demos-sched -V | grep -q debug; then
    skip 0 "DEMOS_SCHEDULE_LOG not supported in release builds" "$_expected_tests"
    exit 0
fi

# Ask demos-sched to store the executed "schedule" in a file
export DEMOS_SCHEDULE_LOG=demos-schedule.log

# One SC process with budget < window length
okx demos-sched -t 20 \
    -C '{windows: [{cpu: 0, length: 10, sc_partition: [
          {cmd: dummy, budget: 5},
        ]}]}'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy' budget=5
    10: cpus=0 cmd='dummy' budget=5
    20: cpus=0 cmd='dummy' budget=5"

# One SC process with budget > window length
okx demos-sched -t 20 \
    -C '{windows: [{cpu: 0, length: 10, sc_partition: [
          {cmd: dummy, budget: 15},
        ]}]}'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy' budget=15
    10: cpus=0 cmd='dummy' budget=15
    20: cpus=0 cmd='dummy' budget=15"

# Two SC processes with budgets < window length
okx demos-sched -t 20 \
    -C '{windows: [{cpu: 0, length: 10, sc_partition: [
          {cmd: dummy 1, budget: 2},
          {cmd: dummy 2, budget: 3},
        ]}]}'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy 1' budget=2
     2: cpus=0 cmd='dummy 2' budget=3
    10: cpus=0 cmd='dummy 1' budget=2
    12: cpus=0 cmd='dummy 2' budget=3
    20: cpus=0 cmd='dummy 1' budget=2"

# One BE process with budget < window length
okx demos-sched -t 20 \
    -C '{windows: [{cpu: 0, length: 10, be_partition: [
          {cmd: dummy, budget: 5},
        ]}]}'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy' budget=5
    10: cpus=0 cmd='dummy' budget=5
    20: cpus=0 cmd='dummy' budget=5"

# One BE process with budget > window length (continues in the next window)
okx demos-sched -t 20 \
    -C '{windows: [{cpu: 0, length: 10, be_partition: [
          {cmd: dummy, budget: 15},
        ]}]}'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy' budget=15
    10: cpus=0 cmd='dummy' budget=5
    20: cpus=0 cmd='dummy' budget=15"

# Two BE processes with budgets < window length
okx demos-sched -t 20 \
    -C '{windows: [{cpu: 0, length: 10, be_partition: [
          {cmd: dummy 1, budget: 2},
          {cmd: dummy 2, budget: 3},
        ]}]}'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy 1' budget=2
     2: cpus=0 cmd='dummy 2' budget=3
    10: cpus=0 cmd='dummy 1' budget=2
    12: cpus=0 cmd='dummy 2' budget=3
    20: cpus=0 cmd='dummy 1' budget=2"

# BE starts only after all SC finish
okx demos-sched -t 20 -C '
windows:
  - length: 10
    slices:
      - cpu: 0
        sc_partition: {cmd: dummy SC1, budget: 5}
      - cpu: 1
        sc_partition: {cmd: dummy SC2, budget: 3}
        be_partition: {cmd: dummy BE_,  budget: 2}
'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy SC1' budget=5
     0: cpus=1 cmd='dummy SC2' budget=3
     5: cpus=1 cmd='dummy BE_' budget=2
    10: cpus=0 cmd='dummy SC1' budget=5
    10: cpus=1 cmd='dummy SC2' budget=3
    15: cpus=1 cmd='dummy BE_' budget=2
    20: cpus=0 cmd='dummy SC1' budget=5
    20: cpus=1 cmd='dummy SC2' budget=3"

# BE with budget continues in the next window
okx demos-sched -t 20 -C '
partitions:
  - name: BE
    processes: [{cmd: dummy BE, budget: 9}]
windows:
  - {length: 2, cpu: 0, be_partition: BE}
  - {length: 3, cpu: 0, be_partition: BE}
'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy BE' budget=9
     2: cpus=0 cmd='dummy BE' budget=7
     5: cpus=0 cmd='dummy BE' budget=4
     7: cpus=0 cmd='dummy BE' budget=2
    10: cpus=0 cmd='dummy BE' budget=9
    12: cpus=0 cmd='dummy BE' budget=7
    15: cpus=0 cmd='dummy BE' budget=4
    17: cpus=0 cmd='dummy BE' budget=2
    20: cpus=0 cmd='dummy BE' budget=9"

# Maximum jitter - this triggers both extremes (reported with the following trace messages):
# - Skipping process with empty effective budget (at time 2 and 4)
# - Process ran out of budget exactly at the window end (at time 6)
DEMOS_RAND_SEED=2 okx demos-sched -t 10 -C 'windows: [{length: 2, cpu: 0, sc_processes: [{cmd: dummy, budget: 1, jitter: 2}]}]'
is "$(< $DEMOS_SCHEDULE_LOG)" "\
     0: cpus=0 cmd='dummy' budget=1
     6: cpus=0 cmd='dummy' budget=2
     8: cpus=0 cmd='dummy' budget=1
    10: cpus=0 cmd='dummy' budget=2"
