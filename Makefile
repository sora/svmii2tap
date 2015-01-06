TARGET_TAP = tapdev
TARGET_TB = tb
TARGET_DPI = tb_svdpi
TARGET_DUT = dut/loopback.v
BUILD_DIR = work

MODELSIM_HOME = /opt/altera/14.1/modelsim_ase

CC = gcc
CFLAGS = -Wall -O

VLIB = vlib
VLOG = vlog
VSIM = vsim
LINT = vlog -lint

all: buildtap buildsim
buildtap: tapdev
buildsim: simlib simlog dpic

dev: tap
sim: vsim
lint: simlint

simlib:
	$(VLIB) $(BUILD_DIR)

simlog: simlib
	$(VLOG) -sv $(TARGET_TB).sv $(TARGET_DUT)

simlint: simlib
	$(LINT) $(TARGET_TB).sv $(TARGET_DUT)

dpic: simlog
	$(CC) -m32 -c -I$(MODELSIM_HOME)/include $(TARGET_DPI).c
	$(CC) -m32 -shared -fPIC -o $(TARGET_DPI).so $(TARGET_DPI).o

vsim:
	$(VSIM) -c -sv_lib $(TARGET_DPI) -do "run -all; quit" $(TARGET_TB)

tap:
	sudo $(TARGET_TAP) pipe0

.PHONY: clean
clean:
	rm -f  *.o $(TARGET_TAP)
	rm -f  *.so transcript *.vcd
	rm -rf $(BUILD_DIR)
