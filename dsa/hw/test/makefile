CC = g++

Source = $(wildcard ./*.cpp)
Object = $(patsubst %.cpp, %.o, $(Source))

CFlags = -Wall -m64
OFlags = -Ofast -march=native -mtune=native
LDFlags = -v

CurrDir = ./
BaseDir = ./..
DetailDir = $(BaseDir)/detail
TestDir = ../../test
TestDetailDir = $(TestDir)/detail
IncludeDir = -I./$(CurrDir) -I./$(TestDir) -I./$(TestDetailDir) -I./$(BaseDir) -I./$(DetailDir)

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