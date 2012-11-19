CC = g++
CFLAGS = -c -Wall -O3
LDFLAGS = -lpthread
SOURCES = threadify.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = threadify

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm $(OBJECTS)

veryclean:
	-rm $(OBJECTS) $(EXECUTABLE) *~