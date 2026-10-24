CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Iinclude -Iinclude/imgui
LIBS = -ld3d9 -lgdi32 -ldwmapi
SRC = src/Nguoi.cpp src/ThiSinh.cpp src/KhuVuc.cpp src/NganhXetTuyen.cpp \
      src/QuanLyThiSinh.cpp src/ConsoleUI.cpp src/Utils.cpp src/AppGUI.cpp \
      src/imgui/imgui.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_tables.cpp \
      src/imgui/imgui_widgets.cpp src/imgui/imgui_impl_win32.cpp src/imgui/imgui_impl_dx9.cpp \
      src/main.cpp
TARGET = QuanLyThiSinh.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LIBS) -o $(TARGET)

clean:
	del /Q $(TARGET) *.o 2>nul || rm -f $(TARGET) *.o
