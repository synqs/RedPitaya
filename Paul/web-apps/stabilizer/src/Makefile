CC=$(CROSS_COMPILE)gcc
RM=rm

OBJECTS=main.o worker.o pid.o fpga_pid.o

CFLAGS+= -Wall -Werror -g -fPIC $(INCLUDE)
CFLAGS += -I../../../../api/include
CFLAGS += -L ../../../../api/lib -lm -lpthread -lrp
LDFLAGS=-shared

CONTROLLER = ../controllerhf.so

all: $(CONTROLLER)

$(CONTROLLER): $(OBJECTS)
	$(CC) -o $(CONTROLLER) $(OBJECTS) $(CFLAGS) $(LDFLAGS)

clean:
	-$(RM) -f $(OBJECTS)
