TOPNAME = top

INC_PATH ?=
VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc \
                -O3 --x-assign fast --x-initial fast --noassert --trace

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)
DUMMY_ELF = $(BUILD_DIR)/dummy-riscv32e-npc.elf

# 修改为根据传入的ELF参数生成inst.hex
build/inst.hex: $(if $(ELF),$(ELF),$(DUMMY_ELF))
	@mkdir -p $(BUILD_DIR)
	riscv64-unknown-elf-objcopy -O verilog --only-section=.text $< $@
	sed -i '/@/d' $@
	
$(DUMMY_ELF):
	@mkdir -p $(BUILD_DIR)
	@echo "int main(){ return 0; }" > $(BUILD_DIR)/dummy.c
	riscv64-unknown-elf-gcc -march=rv32e -mabi=ilp32e -o $@ $(BUILD_DIR)/dummy.c
	@rm $(BUILD_DIR)/dummy.c

default: $(DUMMY_ELF) build/inst.hex $(BIN)

$(shell mkdir -p $(BUILD_DIR))

VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")

INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

$(BIN): $(VSRCS) $(CSRCS)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
        --top-module $(TOPNAME) $(VSRCS) $(CSRCS) \
        $(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
        --Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

all: default

run: $(BIN)
	@mkdir -p build
	@$(BIN) $(if $(ELF),$(ELF),build/dummy-riscv32e-npc.elf)

clean:
	rm -rf $(BUILD_DIR) wave.vcd build/inst.hex

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
