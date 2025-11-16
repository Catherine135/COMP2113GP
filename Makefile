# Makefile for generals_ai project

# Directories
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
CXXFLAGS += -I$(SRC_DIR) -I$(SRC_DIR)/core -I$(SRC_DIR)/admin -I$(SRC_DIR)/render -I$(SRC_DIR)/game_ctrl -I$(SRC_DIR)/human -I$(SRC_DIR)/ai -I$(SRC_DIR)/utils
LDFLAGS = -pthread -lncurses

# Source files
MAIN_SRC = $(SRC_DIR)/main.cpp
CORE_SRCS = $(SRC_DIR)/core/Map.cpp
ADMIN_SRCS = $(SRC_DIR)/admin/Admin.cpp
RENDER_SRCS = $(SRC_DIR)/render/Renderer.cpp $(SRC_DIR)/render/ConsoleRenderer.cpp
GAME_CTRL_SRCS = $(SRC_DIR)/game_ctrl/GameCtrl.cpp
HUMAN_SRCS = $(SRC_DIR)/human/Human.cpp
AI_SRCS = $(SRC_DIR)/ai/GameAI.cpp
UTILS_SRCS = $(SRC_DIR)/utils/Logger.cpp

# All source files
ALL_SRCS = $(MAIN_SRC) $(CORE_SRCS) $(ADMIN_SRCS) $(RENDER_SRCS) $(GAME_CTRL_SRCS) $(HUMAN_SRCS) $(AI_SRCS) $(UTILS_SRCS)
# Object files
MAIN_OBJ = $(OBJ_DIR)/main.o
CORE_OBJS = $(OBJ_DIR)/Map.o
ADMIN_OBJS = $(OBJ_DIR)/Admin.o
RENDER_OBJS = $(OBJ_DIR)/Renderer.o $(OBJ_DIR)/ConsoleRenderer.o
GAME_CTRL_OBJS = $(OBJ_DIR)/GameCtrl.o
HUMAN_OBJS = $(OBJ_DIR)/Human.o
AI_OBJS = $(OBJ_DIR)/GameAI.o
UTILS_OBJS = $(OBJ_DIR)/Logger.o

ALL_OBJS = $(MAIN_OBJ) $(CORE_OBJS) $(ADMIN_OBJS) $(RENDER_OBJS) $(GAME_CTRL_OBJS) $(HUMAN_OBJS) $(AI_OBJS) $(UTILS_OBJS)
# Target executable
TARGET = $(BUILD_DIR)/generals_ai

# Default target
all: $(TARGET)

# Create build directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Link target
$(TARGET): $(ALL_OBJS) | $(OBJ_DIR)
	@echo "Linking $(TARGET)..."
	$(CXX) $(ALL_OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile main.cpp
$(OBJ_DIR)/main.o: $(MAIN_SRC) | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile core sources
$(OBJ_DIR)/Map.o: $(SRC_DIR)/core/Map.cpp $(SRC_DIR)/core/Map.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile admin sources
$(OBJ_DIR)/Admin.o: $(SRC_DIR)/admin/Admin.cpp $(SRC_DIR)/admin/Admin.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile render sources
$(OBJ_DIR)/Renderer.o: $(SRC_DIR)/render/Renderer.cpp $(SRC_DIR)/render/Renderer.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/ConsoleRenderer.o: $(SRC_DIR)/render/ConsoleRenderer.cpp $(SRC_DIR)/render/ConsoleRenderer.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile game_ctrl sources
$(OBJ_DIR)/GameCtrl.o: $(SRC_DIR)/game_ctrl/GameCtrl.cpp $(SRC_DIR)/game_ctrl/GameCtrl.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile human sources
$(OBJ_DIR)/Human.o: $(SRC_DIR)/human/Human.cpp $(SRC_DIR)/human/Human.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ai sources
$(OBJ_DIR)/GameAI.o: $(SRC_DIR)/ai/GameAI.cpp $(SRC_DIR)/ai/GameAI.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile utils sources
$(OBJ_DIR)/Logger.o: $(SRC_DIR)/utils/Logger.cpp $(SRC_DIR)/utils/Logger.h | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Rebuild (clean + build)
rebuild: clean all

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Release build (optimized)
release: CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -DNDEBUG
release: $(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  make          - Build the project (default)"
	@echo "  make clean    - Remove all build artifacts"
	@echo "  make rebuild  - Clean and rebuild"
	@echo "  make run      - Build and run the program"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make release  - Build optimized release version"
	@echo "  make help     - Show this help message"

.PHONY: all clean rebuild run debug release help


