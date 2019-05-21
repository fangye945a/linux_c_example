TARGET := tcp_server
 
INCLUDE:= -I ./include
 
LIBS:= -L ./ -lpthread

CSRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(CSRCS))
 
$(TARGET): $(OBJS) 
	gcc $+ -o $@  $(LIBS)
%.o:%.c
	gcc -c -g $< -o $@ $(INCLUDE)
 
clean:
	rm -rf $(OBJS) $(TARGET)
