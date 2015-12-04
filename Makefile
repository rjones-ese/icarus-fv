.PHONY:run

BUILD_DIR = build
SRC_DIR = src

#AIG Target
AIG_TGT_DIR=$(SRC_DIR)/tgt-aig
AIG_TGT_SRC=$(AIG_TGT_DIR)/aig-target.c\
						$(AIG_TGT_DIR)/aiger.c

AIG_TGT_CONF=$(AIG_TGT_DIR)/aig.conf\
						 $(AIG_TGT_DIR)/aig-s.conf

all: aig.tgt

aig.tgt:
	mkdir -p $(BUILD_DIR)
	gcc -o $(BUILD_DIR)/aig.tgt -fpic -shared $(AIG_TGT_SRC)

run: aig.tgt
	iverilog -o test.aig -t aig examples/simple_fsm/simple_fsm.v

