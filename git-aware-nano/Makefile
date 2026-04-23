CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -fno-stack-protector
CFLAGS   := -O2 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_MEMSTATUS=0
LDFLAGS  := -lpthread

SRC_DIR   := src
BUILD_DIR := build

SRCS     := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/git/*.cpp)
OBJS     := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
SQLITE_O := $(BUILD_DIR)/sqlite/sqlite3.o
TARGET   := $(BUILD_DIR)/nanoclone

# ── Test binaries ─────────────────────────────────────────────────────────

TEST_DIR      := $(SRC_DIR)/tests
TEST_BUILD    := $(BUILD_DIR)/tests

TEST_HL_SRCS  := $(TEST_DIR)/test_highlighters.cpp \
                 $(SRC_DIR)/highlighters.cpp
TEST_HL_BIN   := $(TEST_BUILD)/test_highlighters

TEST_GIT_SRCS := $(TEST_DIR)/test_git.cpp \
                 $(SRC_DIR)/git/git_process.cpp \
                 $(SRC_DIR)/git/git_repo.cpp \
                 $(SRC_DIR)/git/git_diff.cpp \
                 $(SRC_DIR)/git/git_status.cpp \
                 $(SRC_DIR)/git/git_log.cpp
TEST_GIT_BIN  := $(TEST_BUILD)/test_git

.PHONY: all clean run debug test

all: $(TARGET)

$(TARGET): $(OBJS) $(SQLITE_O)
	$(CXX) $(OBJS) $(SQLITE_O) -o $@ $(LDFLAGS)

$(SQLITE_O): $(SRC_DIR)/sqlite/sqlite3.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Generic rule — creates subdirs automatically
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: all

run: all
	$(TARGET)

$(TEST_HL_BIN): $(TEST_HL_SRCS)
	@mkdir -p $(TEST_BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(TEST_HL_SRCS) -o $@

$(TEST_GIT_BIN): $(TEST_GIT_SRCS)
	@mkdir -p $(TEST_BUILD)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(TEST_GIT_SRCS) -o $@ $(LDFLAGS)

test: $(TEST_HL_BIN) $(TEST_GIT_BIN)
	@echo "=== test_highlighters ==="
	@$(TEST_HL_BIN)
	@echo "=== test_git ==="
	@$(TEST_GIT_BIN)

clean:
	rm -rf $(BUILD_DIR)
