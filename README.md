# Round Robin Dispatcher

Simulates Round Robin CPU scheduling with **real Linux processes**.
Each job is a child process made with `fork()` + `exec(./sleepy)`.
The dispatcher controls it with signals:

| Signal    | Meaning                                   |
|-----------|-------------------------------------------|
| `SIGTSTP` | suspend the job when its quantum is over  |
| `SIGCONT` | resume a suspended job                    |
| `SIGINT`  | terminate a finished job                  |

## Build and run

```bash
make                       # builds ./dispatcher and ./sleepy
./dispatcher               # interactive menu
./dispatcher jobs.txt 1    # batch mode: job file + time quantum (default 1)
make clean
```

Run it from this folder (it executes `./sleepy`).
A second terminal can watch live with `tail -F process.log`.

## Files

| File           | What it does                                              |
|----------------|-----------------------------------------------------------|
| `main.c`       | starts the menu, or runs batch mode; Ctrl+C kills children|
| `dispatcher.c` | the Round Robin loop, fork/exec/signals                   |
| `process.c`    | the PCB and reading one job line                          |
| `queue.c`      | FIFO queue (input queue and ready queue)                  |
| `monitor.c`    | live screen output, `log.txt`, `process.log`              |
| `report.c`     | Gantt chart and performance report                        |
| `ui.c`         | interactive menu                                          |
| `sleepy.c`     | the program every child process runs                      |

## Job file format

One job per line: `arrival, priority, cpu_time, memory, r1, r2, r3, r4`.
Only `arrival` and `cpu_time` are used. Blank lines and `#` lines are ignored;
bad lines are skipped with a message.

## Expected result for `jobs.txt`, quantum 1

Average turnaround 10.80 s, waiting 6.80 s, response 0.80 s,
CPU utilisation 100 %, 18 context switches.
