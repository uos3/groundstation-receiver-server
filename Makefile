CC = gcc
CFLAGS = -gdwarf-3 -Og -Wall
#CFLAGS += -D BUILD_VERSION="\"$(shell git describe --dirty --always)\"" \
	-D BUILD_DATE="\"$(shell date '+%Y-%m-%d %H:%M:%S')\""

BIN = receiver-server
SRCS = main.c

LIBS = -lasound -lm
INCLUDES = 

all:
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN) $(LIBS) $(INCLUDES)

clean:
	rm -fv *.o $(BIN)
