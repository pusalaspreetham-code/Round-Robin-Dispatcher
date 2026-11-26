# ===========================================================================
#  Makefile -- Round Robin Dispatcher
# ---------------------------------------------------------------------------
#  make            build the dispatcher and the sleepy job
#  make run        build, then run the simulation with jobs.txt and quantum 1
#  make interactive build, then start the interactive menu (./dispatcher)
#  make clean      delete every generated file (objects, binaries, log.txt)
#
#  NOTE: every recipe line below starts with a TAB character, not spaces.
# ===========================================================================

CC       := gcc
CFLAGS   := -Wall -Wextra -std=gnu11 -g
TARGET   := dispatcher
CHILD    := sleepy
OBJS     := main.o dispatcher.o process.o queue.o monitor.o report.o ui.o
HEADERS  := dispatcher.h process.h queue.h monitor.h report.h ui.h

.PHONY: all run interactive clean

all: $(TARGET) $(CHILD)

# ---- link the dispatcher ----
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# ---- compile any module; every module depends on all headers (small project) ----
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- the job that the dispatcher runs ----
$(CHILD): $(CHILD).c
	$(CC) $(CFLAGS) -o $@ $<

# ---- convenient demo target: one batch run ----
run: all
	./$(TARGET) jobs.txt 1

# ---- start the interactive menu (part 1 + part 2) ----
interactive: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(CHILD) log.txt process.log
