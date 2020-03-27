GXX = g++
DEBUG_FLAG = -ggdb
STATIC_LIB = libreactor.a
DYNAMIC_LIB = libreactor.dylib

TEST_PROG = test
LIBS = -lreactor -losipparser2 -losip2 -lpthread
LIBS_PATH = -L./lib -L/usr/local/lib
INCLUDE_PATH = -I/usr/local/include/osipparser2 -I/usr/local/include/osip2

lib: mk_dir $(STATIC_LIB) #$(DYNAMIC_LIB)

mk_dir:
	mkdir -p lib/

$(STATIC_LIB): reactor.o reactor_impl.o
	ar rcs lib/$(STATIC_LIB) lib/*.o

$(DYNAMIC_LIB): reactor.o reactor_impl.o
	ld -dylib lib/*.o -o lib/$(DYNAMIC_LIB)

reactor.o : src/reactor.cpp
	$(GXX) $(DEBUG_FLAG) -c src/reactor.cpp
	mv reactor.o lib/

reactor_impl.o : src/reactor_impl.cpp
	$(GXX) $(DEBUG_FLAG) -c src/reactor_impl.cpp
	mv reactor_impl.o lib/

$(TEST_PROG) : test.o
	$(GXX) $(DEBUG_FLAG) -o $(TEST_PROG) test.o $(LIBS_PATH) $(LIBS)

test.o : test/test.cpp
	$(GXX) $(DEBUG_FLAG) -c test/test.cpp 

all: mk_dir $(STATIC_LIB) $(DYNAMIC_LIB) test

clean:
	rm lib/*.o lib/$(STATIC_LIB) lib/$(DYNAMIC_LIB) $(TEST_PROG) *.o
