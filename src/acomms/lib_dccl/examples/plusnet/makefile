CXXFLAGS= -I ../..  
VPATH=../..
LIBS=-lxerces-c -ldccl
TARGET=plusnet

$(TARGET): $(TARGET).o $(LIBS) 
	$(CXX) -o $@ $^ $(CXXFLAGS)	

clean:	
	rm -f $(TARGET) *.o