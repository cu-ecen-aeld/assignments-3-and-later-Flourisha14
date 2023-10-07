CC = gcc

TARGET = writer
SOURCE = writer.c

ifdef CROSS_COMPILE
	ifneq ($(shell echo $(CROSS_COMPILE) | grep -- '-$$'),)
    	CC = $(CROSS_COMPILE)gcc
	else
		CC = $(CROSS_COMPILE)
	endif
endif

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)