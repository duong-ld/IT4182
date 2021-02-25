CC = gcc
DEPS = singleLList.h const.h
OBJ = singleLList.o week1.o
TARGET = ex1


all: $(TARGET)
$(TARGET): $(OBJ)
	@echo "[CC] $@"
	@$(CC) -o $@ $^

%.o: %.c $(DEPS)
	@echo "[CC] $@"
	@$(CC) -c -o $@ $<

clean:
	rm -rf ${OBJ}  
	rm -rf ${TARGET}  
