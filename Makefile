GXX = g++
DEBUG_FLAG = -ggdb
STATIC_LIB = libreactor.a
DYNAMIC_LIB = libreactor.so

TEST_PROG = test
LIBS = -lreactor -losipparser2 -losip2 -lpthread
LIBS_PATH = -Llibs -L/usr/local/lib
INCLUDE_PATH = -I/usr/local/include/osipparser2 -I/usr/local/include/osip2

mk_dir:
	mkdir -p libs/

$(STATIC_LIB): reactor.o reactor_impl.o
	ar rcs libs/$(STATIC_LIB) libs/*.o

$(DYNAMIC_LIB): reactor.o reactor_impl.o
	ld -G libs/*.o -o libs/$(DYNAMIC_LIB)

reactor.o : src/reactor.cpp
	$(GXX) $(DEBUG_FLAG) -c src/reactor.cpp
	mv reactor.o libs/

reactor_impl.o : src/reactor_impl.cpp
	$(GXX) $(DEBUG_FLAG) -c src/reactor_impl.cpp
	mv reactor_impl.o libs/

$(TEST_PROG) : test.o
	$(GXX) $(DEBUG_FLAG) -o $(TEST_PROG) test.o $(LIBS_PATH) $(LIBS)

test.o : test/test.cpp
	$(GXX) $(DEBUG_FLAG) -c test/test.cpp 

lib: mk_dir $(STATIC_LIB) $(DYNAMIC_LIB)

all: mk_dir $(STATIC_LIB) $(DYNAMIC_LIB) test

clean:
	rm libs/*.o libs/$(STATIC_LIB) libs/$(DYNAMIC_LIB) $(TEST_PROG) *.o
