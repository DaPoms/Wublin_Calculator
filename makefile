application: main.o wublin.o
	g++ main.o wublin.o -o wublayoutmaker

main.o: main.cpp wublin.h
	g++ -c main.cpp

wublin.o: wublin.cpp wublin.h
	g++ -c wublin.cpp
