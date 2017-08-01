CFLAGS = -c -O2 -Wall -Wextra
LDFLAGS =
TARGET = json_test
OBJECTS = json.o

$(TARGET): json.o
	gcc -o $(TARGET) $< $(LDFLAGS)

%.o: %.c
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(OBJECTS) $(TARGET)

run: $(TARGET)
	curl -s http://json.org/example |head -c1283 |tail -c582 |./json_test

.PHONY: all clean run
