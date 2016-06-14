#Cross compiler definition
CC = $(CROSS_COMPILE)gcc

CFLAGS  = -std=gnu99 -Wall -Werror -g $(INCLUDE)
CFLAGS += -I../../api/include
CFLAGS += -L ../../api/lib -lm -lpthread -lrp


OBJS=main.c

all:
	$(CC) $(CFLAGS) $(OBJS) -o acq_api_test

clean:
	$(RM) *.o
	$(RM) acq_api_test
