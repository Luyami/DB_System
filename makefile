all:
	g++ -c cpp/iohelper.cpp -o bin/iohelper.o -I./include
	g++ -c cpp/db.cpp -o bin/db.o -I./include
	
	ar rcs libcore.a bin/iohelper.o bin/db.o
	
	g++ main.cpp -o main -I./include -L. -lcore