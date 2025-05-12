all:
	g++ -c cpp/iohelper.cpp -o bin/iohelper.o -I./include
	g++ -c cpp/db.cpp -o bin/db.o -I./include
	g++ -c cpp/csv.cpp -o bin/csv.o -I./include

	ar rcs libcore.a bin/iohelper.o bin/db.o bin/csv.o
	
	g++ main.cpp -o main -I./include -L. -lcore