TARGET_NAME=cothread.so
TARGET_SRC=$(wildcard *.c)
TARGET_OBJ=$(TARGET_SRC:.c=.o)

OPT:=

all:$(TARGET_NAME) 
	@echo $< done.
	
CC=gcc
OPT+=-ggdb -O0 -fPIC
FINAL_CFLAGS=$(CFLAGS) $(OPT)
FINAL_LDFLAGS=$(LDFLAGS) $(OPT)
FINAL_LIBS=

$(TARGET_NAME): $(TARGET_OBJ) 
	$(CC) -shared -fPIC -o $@ $(TARGET_OBJ) $(FINAL_LIBS)

%.o: %.c
	$(CC) $(FINAL_CFLAGS) -c $<

clean:
	rm -rf $(TARGET_NAME)  *.o 

.PHONY: clean
