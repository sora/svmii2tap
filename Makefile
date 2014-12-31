TARGET_TAP = tapdev
TARGET_TB = tb
TARGET_DUT = dut/hub.v
BUILD_DIR = work

CC = gcc
CFLAGS = -Wall -O

all: buildtap buildsim

buildtap: tapdev
buildsim: vlib vlog dpic
dev: tap
sim: vsim

vlib:
	vlib $(BUILD_DIR)

vlog:
	vlog $(TARGET_TB).sv $(TARGET_DUT)

dpic:
	$(CC) -m32 -c -I$(MODELSIM_HOME)/include $(TARGET_TB).c
	$(CC) -m32 -shared -fPIC -o $(TARGET_TB).so $(TARGET_TB).o

vsim:
	vsim -c -sv_lib $(TARGET_TB) -do "run -all; quit" $(TARGET_TB)

tap:
	sudo $(TARGET_TAP) pipe0

.PHONY: clean
clean:
	rm -f  *.o $(TARGET_TAP)
	rm -f  *.so transcript
	rm -rf $(BUILD_DIR)
