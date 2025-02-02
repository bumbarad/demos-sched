#!/usr/bin/env bash
. testlib
plan_tests 7

api-test
is $? 1 "Running outside of demos-sched fails"

# Check that demos schedules first two iterations of the sub-process
readarray -t lines < <(demos-sched -C "{
    windows: [ {length: 50, slices: [ { cpu: 0, sc_partition: SC1 }] } ],
    partitions: [ { name: SC1, processes: [ { cmd: api-test first second, budget: 500 } ] } ]
}")
echo PATH=$PATH >&2
is "${lines[0]}" "first" "first iteration"
is "${lines[1]}" "second" "second iteration"


# Check that demos doesn't hang when process exits during initialization
# 3 seconds should be long enough, but fundamentally, it's a race condition
out=$(timeout 3s demos-sched -C '
windows:
  - length: 1
    slices:
      - cpu: 0
        sc_partition: SC1

partitions:
  - name: SC1
    processes:
      - cmd: exit
        budget: 100
        init: yes
      - cmd: exit
        budget: 100
')
ok $? "Does not hang on process exit during initialization"


# Check that each process has opportunity to initialize before window scheduler starts
# this assumes that your shell (/bin/sh) starts up in under ~40ms
# alternative would be to make the window longer (and adjust sleep in api-init-test),
#  but that would make the test itself longer
out=$(demos-sched -m "<test>" -C '
windows:
  - length: 50
    slices:
      - {cpu: 0, sc_partition: SC1}
      - {cpu: 1, sc_partition: SC2}

partitions:
  - name: SC1
    processes: [{cmd: api-init-test, budget: 50, init: yes}]
  - name: SC2
    processes: [{cmd: api-init-test, budget: 50, init: yes}]
')
is "$out" \
"init start
init start
init done
init done
<test>"


out=$(timeout 3s demos-sched -m "<win>" -C '
windows:
  - length: 100
    slices:
      - {cpu: 0, sc_processes: {init: yes,
            cmd: "echo init && demos-signal-completion && echo done"}}
')
ok $? "Calling 'demos-signal-completion' ends initialization"
is "$out" \
"init
<win>
done" "Calling 'demos-signal-completion' ends initialization"