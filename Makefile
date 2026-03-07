TARGET = build/main.exe

all: $(TARGET)

$(TARGET): src/main.cpp
	g++ -std=c++23 -g \
	-Ic:/raylib/raylib/src \
	-Ic:/RaylibProjects/blendspace/include \
	src/main.cpp \
	-o $(TARGET) \
	-Lc:/raylib/raylib/src \
	-lraylib -lopengl32 -lgdi32 -lwinmm

clean:
	del /Q build/main
