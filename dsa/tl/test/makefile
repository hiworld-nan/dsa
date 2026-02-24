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
IncludeDir = -I./$(CurrDir) -I./$(TestDir) -I./$(BaseDir) -I./$(DetailDir)

all: tl

$(Object):%.o: %.cpp
	$(CC) $(CFlags) $(IncludeDir) -c $< -o $@

tl: $(Object)
	$(CC) -o $@ $^ $(LDFlags)
	$(shell [ ! -d ./bin ] && mkdir -p ./bin )
	mv $@ ./bin && mv $(Object) ./bin

.PHONY: clean
clean:
	-rm -f ./bin/*