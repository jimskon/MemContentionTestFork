# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -lrt -lpthread

# Target executable
TARGET = memory_test_fork

# Source files
SRCS = memory_test.c

# Default rule
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

# Clean rule to remove the executable and any shared memory object left
clean:
	rm -f $(TARGET)
	rm -f /dev/shm/shared_mem  # Clean up shared memory if needed

# Phony targets (do not treat as files)
.PHONY: all clean
