CC = gcc
DEPS = linkList.h constain.h
OBJ = linkList.o main.o
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
