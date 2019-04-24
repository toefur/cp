CFLAGS=-g -std=c99 -Wall -pedantic -pthread

TARGET = cp

# define directories
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# define file locations
SOURCES := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# build the binary and store it in bin/
$(BINDIR)/$(TARGET): $(OBJECTS)
	gcc $(CFLAGS) $(OBJECTS) -o $@

# build objects and store them in obj/
$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	gcc $(CFLAGS) -c $< -o $@

# clean out the object files
clean:
	rm -rf $(OBJECTS)

# clean out the object files and remove the binary
remove: clean
	rm -rf $(BINDIR)/$(TARGET)
