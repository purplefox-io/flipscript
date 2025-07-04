# FlipScript Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = 

# Source files
SRCS = main.c lexer.c parser.c compiler.c codegen.c runtime.c
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = flipscript

# Build the executable
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Rule for building object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony targets
.PHONY: clean all test

# Clean the build
clean:
	rm -f $(OBJS) $(TARGET)

# Test with a simple FlipScript example
test: $(TARGET)
	@echo "Testing FlipScript with example.fs"
	@echo 'import furi\n\ncfunc furi_get_tick(): "furi_get_tick"\ncfunc furi_delay(ms): "furi_delay"\n\ndef main():\n    start = furi_get_tick()\n    print("Hello from FlipScript!")\n    furi_delay(1000)\n    end = furi_get_tick()\n    print(f"Execution time: {end - start}ms")\n\nmain()' > example.fs
	./$(TARGET) -c -o example.c example.fs
	@echo "Generated C code in example.c"
	./$(TARGET) -b -o example.fsb example.fs
	@echo "Generated bytecode in example.fsb"

# Build for Flipper Zero target
# Note: This requires the Flipper Zero SDK to be set up
flipper: $(SRCS) flipper_main.c
	@echo "Building for Flipper Zero target"
	# Replace this with the actual Flipper Zero build command
	# For example: $(FLIPPER_SDK)/scripts/fbt APPID=flipscript