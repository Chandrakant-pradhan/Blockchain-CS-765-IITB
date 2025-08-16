CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -g
LDFLAGS = 

SRC_DIR = src
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(SRC_FILES:.cpp=.o)
TARGET = simulation

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) -o $@ $^ -lcrypto


$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
	rm -f *.txt $(TARGET) *.png
