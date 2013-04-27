SRCS:=$(wildcard *.cpp)
OBJS:=$(SRCS:.cpp=.o)
DEPS:=$(SRCS:.cpp=.d)
CC:=g++
PROJECT:=noj

.PHONY:all clean

all:main

%.d:%.cpp
	rm -f $@; \
	$(CC) -MM $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

main:$(OBJS)
	$(CC) -o $(PROJECT) $(OBJS) -L. -I. -Wall -g

#ifneq ($(MAKECMDGOALS), clean)
#	include $(DEPS)
#endif

clean:
	rm -f *.d *.d.* *.o *.a $(PROJECT)
