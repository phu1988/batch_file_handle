CPP          = g++
CPP_FLAG     = -lpthread -g -D DDEBUG
#CPP_FLAG     = -lpthread -g
srcs         = $(wildcard *.cpp)
objs         = $(srcs:.cpp=.o)
bin          = ./BatchFile
LIBS         = -L$(RISKMAN_ROOT)/lib -lxdbo2
INCLS        = -I$(RISKMAN_ROOT)/include -I../include

.PHONY: all clean
all: $(bin)
	@echo "All made!"

$(bin): $(objs)
	$(CPP) $(CPP_FLAG) -o $@ $(objs) $(LIBS)

.cpp.o:
	$(CPP) $(CPP_FLAG) $(INCLS) -c $<

clean:
	@echo "[CLEAN] Deleting $(bin)"
	@$(RM) $(bin)
	@echo "[CLEAN] Deleting $(objs)"
	@$(RM) $(objs)
