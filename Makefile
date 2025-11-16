# Compiler
CC = gcc

# Flags
CFLAGS = -Wall -Wextra -std=c99 -O2

# Target
TARGET = main

# Source files
SRCS = $(wildcard src/**/*.c) $(wildcard src/*.c)

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)
