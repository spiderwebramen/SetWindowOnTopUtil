SHELL = powershell.exe

cleanbuild:
	gcc -Wall -Wextra -c main.c -o "build/main.o"
	windres -i resource.rc --input-format=rc -o "build/resource.o" -O coff
	gcc -mwindows "build/main.o" "build/resource.o" -o "build/SetWindowOnTop.exe" -lwinmm
