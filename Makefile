GXX = g++
FLAG = -ggdb -std=c++11
STATIC_LIB = libreactor.a
DYNAMIC_LIB = libreactor.dylib

TEST = test
LIBS = -lreactor -losipparser2 -losip2 -lpthread
LIBS_PATH = -L./lib -L/usr/local/lib

OBJECTS = reactor.o reactor_impl.o connection_acceptor.o select_reactor_impl.o \
					poll_reactor_impl.o epoll_reactor_impl.o devpoll_reactor_impl.o \
					kqueue_reactor_impl.o tcp_handler.o udp_handler.o

.PHONY: lib
lib: mk_dir $(STATIC_LIB) $(DYNAMIC_LIB)
	mv $(OBJECTS) lib

mk_dir:
	mkdir -p lib

$(STATIC_LIB): $(OBJECTS)
	ar rcs lib/$(STATIC_LIB) $(OBJECTS)

$(DYNAMIC_LIB): $(OBJECTS)
	$(GXX) -dynamiclib $(OBJECTS) -o lib/$(DYNAMIC_LIB)

reactor.o : src/reactor.cpp
	$(GXX) $(FLAG) -c src/reactor.cpp

reactor_impl.o : src/reactor_impl.cpp
	$(GXX) $(FLAG) -c src/reactor_impl.cpp

connection_acceptor.o : src/connection_acceptor.cpp
	$(GXX) $(FLAG) -c src/connection_acceptor.cpp

select_reactor_impl.o : src/select_reactor_impl.cpp
	$(GXX) $(FLAG) -c src/select_reactor_impl.cpp

poll_reactor_impl.o : src/poll_reactor_impl.cpp
	$(GXX) $(FLAG) -c src/poll_reactor_impl.cpp

epoll_reactor_impl.o : src/epoll_reactor_impl.cpp
	$(GXX) $(FLAG) -c src/epoll_reactor_impl.cpp

devpoll_reactor_impl.o : src/devpoll_reactor_impl.cpp
	$(GXX) $(FLAG) -c src/devpoll_reactor_impl.cpp

kqueue_reactor_impl.o : src/kqueue_reactor_impl.cpp
	$(GXX) $(FLAG) -c src/kqueue_reactor_impl.cpp

tcp_handler.o : src/tcp_handler.cpp
	$(GXX) $(FLAG) -c src/tcp_handler.cpp

udp_handler.o : src/udp_handler.cpp
	$(GXX) $(FLAG) -c src/udp_handler.cpp

timer.o : src/timer.cpp
	$(GXX) $(FLAG) -c src/timer.cpp

$(TEST) : test.o
	$(GXX) $(FLAG) -o $(TEST) test.o $(LIBS_PATH) $(LIBS)

test.o : test/test.cpp
	$(GXX) $(FLAG) -c test/test.cpp 

all: lib $(TEST)

.PHONY: clean
clean:
	cd lib && \
	rm $(OBJECTS) $(STATIC_LIB) $(DYNAMIC_LIB) ../test/$(TEST) ../test/*.o
