CC=gcc
CFLAGS=-O3 -funroll-loops -c
LDFLAGS=-O2
LDLIBS=-lm
SOURCES=lebot.c MyBot.c ants.c
HEADERS=ants.h util.h
OBJECTS=$(addsuffix .o, $(basename ${SOURCES}))
EXECUTABLE=MyBot

all: $(EXECUTABLE)

zip: $(SOURCES) Makefile $(HEADERS)
	zip -r mypackage.zip $^

$(EXECUTABLE): $(OBJECTS)

$(OBJECTS): %.o: %.c $(HEADERS)

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

.PHONY: all clean
