# Paths
OBJ_DIR = ./src
INC_DIR = ./include
TEXT_DIR = ./texts
TEST_DIR = ./tests

# Flags and compiler
CFLAGS = -Wall  -I$(INC_DIR)
CC = gcc

# Objects
COMMON_OBJS = $(OBJ_DIR)/utilities.o
OBJ_PARENT = $(OBJ_DIR)/parent.o
OBJ_CHILD = $(OBJ_DIR)/child.o
OBJ_TEST = $(TEST_DIR)/test_utilities.o
OBJS = $(COMMON_OBJS) $(OBJ_PARENT) $(OBJ_CHILD)

# Executables
EXEC_PARENT = parent
EXEC_CHILD = child
EXEC_TEST = test

# Define all targets
all: target_parent target_child target_test

# And each one individually
target_parent: $(EXEC_PARENT)

target_child: $(EXEC_CHILD)

target_test: $(EXEC_TEST)

$(EXEC_PARENT): $(COMMON_OBJS) $(OBJ_PARENT)
	@$(CC) $(CFLAGS) $(COMMON_OBJS) $(OBJ_PARENT) -lpthread -o $(EXEC_PARENT)

$(EXEC_CHILD): $(COMMON_OBJS) $(OBJ_CHILD)
	@$(CC) $(CFLAGS) $(COMMON_OBJS) $(OBJ_CHILD) -lpthread -o $(EXEC_CHILD)

$(EXEC_TEST): $(COMMON_OBJS) $(OBJ_TEST)
	@$(CC) $(CFLAGS) $(COMMON_OBJS) $(OBJ_TEST) -o $(EXEC_TEST)

.SILENT: $(OBJS) # Silence implicit rule output
.PHONY: clean

clean:
	@echo "Cleaning up ..."
	@rm -f $(OBJ_TEST) $(OBJS) $(EXEC_PARENT) $(EXEC_CHILD) $(EXEC_TEST)
	@rm -f results/*.txt
	@rm -f ./*.txt

run: $(EXEC_PARENT)
	@./$(EXEC_PARENT) $(TEXT_DIR)/$(text) $(children) $(transactions) $(lines_per_segment)	# Taking args from command line
	@mv *.txt results

run-tests: $(EXEC_TEST)
	@./$(EXEC_TEST)
