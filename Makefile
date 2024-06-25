SHELL = powershell.exe
CFLAGS += -Wall -Wextra 

all: what link

what:
	mkdir -Force build
link: main.o resource.o
	gcc $(CFLAGS) -mwindows "build/main.o" "build/resource.o" -o "build/SetWindowOnTop.exe" -lwinmm
main.o: main.c
	gcc $(CFLAGS) -c main.c -o "build/main.o"
resource.o: resource.rc resource.h
	windres -i resource.rc --input-format=rc -o "build/resource.o" -O coff

clean:
	rm -r build/
