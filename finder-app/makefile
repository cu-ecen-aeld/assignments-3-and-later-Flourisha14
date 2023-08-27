CC = gcc

TARGET = writer
SOURCE = writer.c

ifdef CROSS_COMPILE
    CC = $(CROSS_COMPILE)gcc
endif

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)