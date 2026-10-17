CXX = g++
CXXFLAGS = -std=c++11 -Wall -Iinclude
SRC = src/Nguoi.cpp src/ThiSinh.cpp src/QuanLyThiSinh.cpp src/ConsoleUI.cpp src/Utils.cpp src/main.cpp
TARGET = QuanLyThiSinh.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	del /Q $(TARGET) *.o 2>nul || rm -f $(TARGET) *.o
