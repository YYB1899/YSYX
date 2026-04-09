#get platform
ARCH_SPLIT = $(subst -, ,$(ARCH))
PLATFORM   = $(word 2,$(ARCH_SPLIT))

TOPNAME = ysyxtop

INC_PATH = $(NPC_HOME)/include 
#add vsrc and csrc
VSOURCES=$(shell find $(abspath ./vsrc/CPU) -name "*.v" -or -name "*.sv" )
CSOURCES=$(shell find $(abspath ./csrc/cpu) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
INC_VDEFINE = $(NPC_HOME)/vsrc/CPU/define


#build directory
BUILD_DIR = $(NPC_HOME)/build
$(shell mkdir -p $(BUILD_DIR))
# lint off warning
Warning = ASSIGNDLY DECLFILENAME PINCONNECTEMPTY  UNDRIVEN SYNCASYNCNET  UNUSEDSIGNAL DEFPARAM UNUSEDPARAM
VERILATOR_FLAGS += --timescale "1ns/1ns" --no-timing --autoflush

#PLATFORM从ARCH变量里提取。若不设置ARCH，则无法触发NPC宏，导致RTL里io_master系列信号未声明，产生IMPLICIT警告。
ifeq ($(PLATFORM),npc)
VERILATOR_FLAGS += -GResetValue=0x80000000 -DNPC 
VSOURCES +=$(shell find $(abspath ./vsrc/SRAM) -name "*.v" -or -name "*.sv" )
endif

OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

DIFF_REF_SO = $(NEMU_HOME)/build/riscv32-nemu-interpreter-so
ARGS_DIFF = -d $(DIFF_REF_SO)

VERILATOR = verilator
# -- assert allows all assert -sv allows SV parses
VERILATOR_FLAGS += -Wall --cc -MMD --build --trace $(addprefix -Wno-, $(Warning)) --assert  \
			-O2 --x-assign fast --x-initial fast  $(addprefix -I, $(INC_VDEFINE))


LDFLAGS = -lSDL2 -lSDL2_image -O2 -lreadline -ldl -pie 


INCFLAGS = $(addprefix -I, $(INC_PATH))

CFLAGS  = -O2 -MMD -Wall $(INCFLAGS) -D__$(shell echo $(PLATFORM) | tr a-z A-Z)__ -DTOP_NAME="\"V$(TOPNAME)\""

# llvm lib
LDFLAGS += $(shell llvm-config --libs) -fsanitize=address 

CFLAGS += -I/usr/lib/llvm-14/include -std=c++14 -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -fPIE -rdynamic



IMG ?= 

override ARGS += $(ARGS_DIFF)

NPC_EXEC = $(BIN) $(ARGS) $(IMG)


NVBOARD_ARCHIVE ?= 
MODULE ?=

sim:all
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	$(NPC_EXEC)

gdb: all
	$(call git_commit, "gdb NEMU")
	gdb -s $(BIN) --args $(NPC_EXEC)

all:$(CSOURCES) $(VSOURCES)  $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR) *.vcd
	$(VERILATOR) $(VERILATOR_FLAGS) \
	--top-module $(TOPNAME) \
	$(addprefix -CFLAGS , $(CFLAGS))  $(addprefix -LDFLAGS , $(LDFLAGS)) \
	$^ \
	--Mdir $(OBJ_DIR)  --exe -o $(abspath $(BIN))\


clean: 
	rm -rf $(BUILD_DIR) *.vcd
	$(MAKE) -C ~/yosys-sta clean

.PHONY: clean all sim gdb

include ../Makefile
