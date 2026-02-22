objs=src/network/network.c src/core/window.c
libs=-luser32 -lcomctl32
includes=\
-I src \
-I src/core \
-I src/network

all:
	clang $(objs) main.c -o client.exe $(includes) $(libs)

clean:
	rm -rf *.exe