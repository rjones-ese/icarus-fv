.PHONY: clean

BUILD_DIR?=../../build
VPI_OBJ?=test

all:
	iverilog -osimple_fsm.vvp -s -t../fv-vpi_test simple_fsm_tb simple_fsm_tb.v simple_fsm.v

run:
	vvp -M$(BUILD_DIR) -m$(VPI_OBJ) simple_fsm.vvp

clean:
	rm *.vvp
