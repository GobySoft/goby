# use to compile lib_dccl as a standalone (no MOOS)
CXXFLAGS=-Wall -I . -I ../lib_tes_util -fPIC -g

LIB=libdccl.so
LIBSRC= dccl.cpp message_algorithms.cpp message.cpp message_publish.cpp message_queuing.cpp message_val.cpp message_var.cpp message_xml_callbacks.cpp message_xml_parser.cpp
LIBOBJ= $(LIBSRC:.cpp=.o)


all: $(LIB) examples 


$(LIB): $(LIBOBJ) 
	$(CXX) -shared -o $(LIB) $(LIBOBJ)

.PHONY: examples
examples:
	cd examples && $(MAKE)

.PHONY: install
install:
	cp $(LIB) /usr/lib

.PHONY: clean
clean:
	rm -f $(LIBOBJ) $(LIB)
	cd examples && $(MAKE) clean