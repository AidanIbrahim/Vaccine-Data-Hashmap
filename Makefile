CXXFLAGS = -Wall -g -fexec-charset=utf-8 
CXX = g++
DEBUG = gdb
EXECUTABLE = proj4


proj4: mytest.o vacdb.o
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) vacdb.o mytest.o

mytest.o: mytest.cpp
	$(CXX) $(CXXFLAGS) -c mytest.cpp

vacdb.o: vacdb.cpp vacdb.h
	$(CXX) $(CXXFLAGS) -c vacdb.cpp

clean: 
	rm *.o proj4 *~

debug: proj4 
	$(DEBUG) $(EXECUTABLE)