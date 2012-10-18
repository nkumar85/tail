CC=gcc
CFLAGS = -g -Wall
EXETARGET = my_tail
OBJS = tail_file_scan.o tail_wrap.o tail_main.o

.PHONY : rebuild all clean

all: $(OBJS)
	 $(CC) $(OBJS) -o $(EXETARGET)

clean:
	$(shell rm -f $(OBJS) $(EXETARGET) 2>&1>/dev/null)

rebuild: clean all
