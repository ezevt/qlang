CC := clang
TARGET := qlang

SRC_DIR := src
BUILD_DIR := build
TEST_DIR := tests



CFLAGS := -std=c11 \
				 -Wall -Wextra -Wpedantic \
				 -Wshadow -Wconversion -Wstrict-prototypes \
				 -Wmissing-prototypes \
				 -D_GNU_SOURCE \
				 -I$(SRC_DIR)

DEPFLAGS := -MMD -MP



SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

BIN := $(BUILD_DIR)/$(TARGET)



.PHONY: all
all: $(BIN)

$(BIN): $(OBJS) | $(BUILD_DIR)
	@echo "  LINK   $@"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "  CC     $<"
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@



.PHONY: run
run: $(BIN)
	@./$(BIN)

.PHONY: run-file
run-file: $(BIN)
	@./$(BIN) $(FILE)



-include $(DEPS)
