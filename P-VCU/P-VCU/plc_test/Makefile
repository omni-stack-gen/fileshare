CC :=/opt/X5/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
TEST_SRC  := plc_ctrl.c spi_plc.c
TEST_OBJ  := $(TEST_SRC:%.c=%.o)

TARGET := plc_test
.PHONY : clean all test


all: test

test: $(TARGET)

$(TARGET): $(TEST_OBJ)
		$(CC) -pthread  $^ -o $@ -static

clean:
	@rm -f $(TARGET)
	@rm -f $(TEST_OBJ)
	@rm -f $(TARGET)
