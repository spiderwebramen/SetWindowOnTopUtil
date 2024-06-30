SHELL = powershell.exe
CFLAGS += -Wall -Wextra

build: link
all: mkdir link strip

link: main.o resource.o
	gcc $(CFLAGS) -static -s -mwindows "build/main.o" "build/resource.o" -o "build/SetWindowOnTop.exe" -lwinmm
main.o: main.c
	gcc $(CFLAGS) -Os -c main.c -o "build/main.o"
resource.o: resource.rc resource.h
	windres -i resource.rc --input-format=rc -o "build/resource.o" -O coff

mkdir:
	mkdir -Force build
clean:
	rm -r build/
strip:
	strip -s -R .comment -R .gnu.version --strip-unneeded "build/SetWindowOnTop.exe"
