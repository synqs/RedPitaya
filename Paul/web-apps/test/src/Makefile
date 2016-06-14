CC=$(CROSS_COMPILE)gcc
RM=rm

OBJECTS=main.o 

CFLAGS+= -Wall -Werror -g -fPIC $(INCLUDE)
LDFLAGS=-shared

CONTROLLER = ../controllerhf.so

all: $(CONTROLLER)

$(CONTROLLER): $(OBJECTS)
	$(CC) -o $(CONTROLLER) $(OBJECTS) $(CFLAGS) $(LDFLAGS)

clean:
	-$(RM) -f $(OBJECTS)
