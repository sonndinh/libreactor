#libreactor.so : Reactor.o ReactorImpl.o
#	
#########################################

GXX = g++
DEBUG_FLAG = -ggdb
STATIC_LIB = libreactor.a
DYNAMIC_LIB = libreactor.so

mk_dir:
	mkdir -p libs/

$(STATIC_LIB): Reactor.o ReactorImpl.o
	ar rcs libs/$(STATIC_LIB) libs/*.o

$(DYNAMIC_LIB): Reactor.o ReactorImpl.o
	ld -G libs/*.o -o libs/$(DYNAMIC_LIB)

Reactor.o : Reactor.cpp
	$(GXX) $(DEBUG_FLAG) -c src/Reactor.cpp -I$(FASTFORMAT_ROOT)/include -I$(STLSOFT)/include
	mv Reactor.o libs/

ReactorImpl.o : ReactorImpl.cpp
	$(GXX) $(DEBUG_FLAG) -c src/ReactorImpl.cpp -I$(FASTFORMAT_ROOT)/include -I$(STLSOFT)/include
	mv ReactorImpl.o libs/

all: mk_dir $(TARGET_LIB)

clean:
	rm libs/*.o libs/$(STATIC_LIB) libs/$(DYNAMIC_LIB)


#	Build a test. 
TEST_PROG = test

LIBS = -lreactor -lpantheios.1.core.gcc43 -lpantheios.1.be.fprintf.gcc43 \
	-lpantheios.1.bec.fprintf.gcc43 -lpantheios.1.fe.all.gcc43 \
	-lpantheios.1.util.gcc43 -losipparser2 -losip2 -lpthread

LIBS_PATH = -Llibs -L$(FASTFORMAT_ROOT)/lib -L/usr/local/lib

INCLUDE_PATH = -I$(FASTFORMAT_ROOT)/include \
	-I$(STLSOFT)/include -I/usr/local/include/osipparser2 \
	-I/usr/local/include/osip2

$(TEST_PROG) : test.o
	$(GXX) $(DEBUG_FLAG) -o $(TEST_PROG) test.o $(LIBS_PATH) $(LIBS)

main.o : test.cpp
	$(GXX) $(DEBUG_FLAG) -c test.cpp 

clean:
	rm $(TEST_PROG) *.o

#################################################
#test.exe : main.o Reactor.o ReactorImpl.o
#	g++ -fPIC -o test.exe main.o Reactor.o ReactorImpl.o -L$(FASTFORMAT_ROOT)/lib \
#		-lpantheios.1.core.gcc43\
#		-lpantheios.1.be.fprintf.gcc43 -lpantheios.1.bec.fprintf.gcc43\
#		-lpantheios.1.fe.all.gcc43 -lpantheios.1.util.gcc43
#
#main.o : main.c
#	g++ -c main.c -I$(FASTFORMAT_ROOT)/include -I$(STLSOFT)/include
#Reactor.o : Reactor.c
#	g++ -c Reactor.c -I$(FASTFORMAT_ROOT)/include -I$(STLSOFT)/include
#ReactorImpl.o : ReactorImpl.c
#	g++ -c ReactorImpl.c -I$(FASTFORMAT_ROOT)/include -I$(STLSOFT)/include
#clean:
#	rm test.exe main.o Reactor.o ReactorImpl.o
##################################################
