CXXFLAGS= -I ../.. 
VPATH=../..
LIBS=-lxerces-c -ldccl
TARGET=two_message

$(TARGET): $(TARGET).o $(LIBS) 
	$(CXX) -o $@ $^ $(CXXFLAGS)	

clean:	
	rm -f $(TARGET) *.o