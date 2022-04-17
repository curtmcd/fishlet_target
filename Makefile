CC = g++

CAIRO_CFLAGS = $(shell pkg-config --cflags cairo)
CAIRO_LIBS = $(shell pkg-config --libs cairo)

CFLAGS_DEBUG = -DDEBUG -g
CFLAGS = -Wall -Werror $(CFLAGS_DEBUG) $(CAIRO_CFLAGS)
LIBS = $(CAIRO_LIBS)
INCLUDES = -I..

SIZES = 8.5x11 11x8.5 11x17 17x11

TARGETS = $(foreach s,$(SIZES),target-$(s).pdf)

all: $(TARGETS)

target-%.pdf: target
	SIZE=`echo $@ | sed -e s,target-,, -e s,\.pdf,,`; \
	./target -s $$SIZE -o $@

TARGET_SRC = target.cpp

target: $(TARGET_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o target $(TARGET_SRC) $(LIBS)

.PHONY: clean
clean:
	$(RM) *.pdf target
