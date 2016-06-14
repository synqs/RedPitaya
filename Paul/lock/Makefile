#Cross compiler definition
CC = $(CROSS_COMPILE)gcc

CFLAGS  = -std=gnu99 -Wall -Werror -g $(INCLUDE)
CFLAGS += -I../../api/include
CFLAGS += -L ../../api/lib -lm -lpthread -lrp


OBJS=main.c fpga_pid.c pid.c

all:
	$(CC) $(CFLAGS) $(OBJS) -o lock

clean:
	$(RM) *.o
	$(RM) lock
