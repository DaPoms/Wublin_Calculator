application: main.obj wublin.obj
	cl main.obj wublin.obj -o wublayoutmaker

main.obj: main.cpp wublin.h
	cl -c main.cpp

wublin.obj: wublin.cpp wublin.h
	cl -c wublin.cpp
