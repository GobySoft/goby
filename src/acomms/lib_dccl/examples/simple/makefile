CXXFLAGS= -I ../.. -g
VPATH=../..
LIBS=-lxerces-c -ldccl
TARGET=simple

$(TARGET): $(TARGET).o $(LIBS) 
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:	
	rm -f $(TARGET) *.o