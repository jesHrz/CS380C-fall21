CXX := g++

TARGET := translator

CXX_FLAGS := -std=c++14 -g
CXX_FILES := main.cpp misc.cpp
HEADER_FILES := misc.h


all: $(TARGET)

$(TARGET): $(CXX_FILES)
	$(CXX) $(CXX_FLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET)
