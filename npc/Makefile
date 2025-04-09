TOPNAME = top

INC_PATH ?=
VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc \
                -O3 --x-assign fast --x-initial fast --noassert --trace --sv --exe

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)
DUMMY_ELF = $(BUILD_DIR)/dummy-riscv32e-npc.elf
CXXFLAGS += -g -ggdb3  # 添加调试信息
VERILATOR_CFLAGS += --debug
# 添加DiffTest编译选项
DIFFTEST_REF ?= $(NEMU_HOME)/build/riscv32-nemu-interpreter-so
CXXFLAGS += -DDIFFTEST_ON
# Capstone 相关配置
CAPSTONE_DIR = /home/yyb/ysyx-workbench/nemu/tools/capstone/repo
CAPSTONE_LIB = $(CAPSTONE_DIR)/libcapstone.a

LDFLAGS += $(CAPSTONE_DIR)/libcapstone.a -ldl
CFLAGS += -I/home/yyb/ysyx-workbench/nemu/tools/capstone/repo/include

# 修改为根据传入的ELF参数生成inst.hex
build/inst.hex: $(if $(ELF),$(ELF),$(DUMMY_ELF))
	@mkdir -p $(BUILD_DIR)
	@echo "Generating inst.hex from $<"
	riscv64-unknown-elf-objcopy -O verilog --only-section=.text $< $@ || (echo "Objcopy failed"; exit 1)
	sed -i '/@/d' $@
	@test -s $@ || (echo "Generated inst.hex is empty"; exit 1)
		
$(DUMMY_ELF):
	@mkdir -p $(BUILD_DIR)
	@echo "int main(){ return 0; }" > $(BUILD_DIR)/dummy.c
	riscv64-unknown-elf-gcc -g -ggdb3 -march=rv32e -mabi=ilp32e \
		-nostdlib -nostartfiles -ffreestanding \
		-o $@ $(BUILD_DIR)/dummy.c
	@rm $(BUILD_DIR)/dummy.c
debug: CXXFLAGS += -DDEBUG -O0  # 禁用优化以便更好调试
debug: VERILATOR_CFLAGS += --debug
debug: default
default: $(DUMMY_ELF) build/inst.hex $(BIN)

$(shell mkdir -p $(BUILD_DIR))

VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")

INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

# 添加 Capstone 库作为依赖
$(CAPSTONE_LIB):
	$(MAKE) -C $(CAPSTONE_DIR)

$(BIN): $(VSRCS) $(CSRCS) $(CAPSTONE_LIB)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
        --top-module $(TOPNAME) $(VSRCS) $(CSRCS) \
        $(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
        --Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

all: default

run: $(BIN) build/inst.hex
	@mkdir -p build
	@$(BIN) $(if $(ELF),$(ELF),build/dummy-riscv32e-npc.elf) $(if $(DEBUG),--debug,) --diff=$(DIFFTEST_REF)

clean:
	rm -rf $(BUILD_DIR) wave.vcd build/inst.hex 

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
