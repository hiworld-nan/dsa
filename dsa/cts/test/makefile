CC = g++

Source = $(wildcard ./*.cpp)
Object = $(patsubst %.cpp, %.o, $(Source))

CFlags = -Wall -m64 -std=c++2c
OFlags = -Ofast -march=native -mtune=native
LDFlags = -v

CurrDir = ./
BaseDir = ./..
TestDir = ../../test
IncludeDir = -I./$(CurrDir) -I./$(TestDir) -I./$(BaseDir)

all: test

$(Object):%.o: %.cpp
	$(CC) $(CFlags) $(IncludeDir) -c $< -o $@

test: $(Object)
	$(CC) -o $@ $^ $(LDFlags)
	$(shell [ ! -d ./bin ] && mkdir -p ./bin )
	mv $@ ./bin && mv $(Object) ./bin

.PHONY: clean
clean:
	-rm -f ./bin/*