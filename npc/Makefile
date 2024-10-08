VERILATOR = verilator
VERILATOR_FLAGS = -Wall $(SRC_DIR)/top.v $(SRC_DIR)/top_main.cpp --cc --trace --exe --build
SRC_DIR = ../../verilator/examples/switch
OBJ_DIR = $(SRC_DIR)/obj_dir
EXECUTABLE = $(OBJ_DIR)/Vtop
WAVE_VCD = $(SRC_DIR)/wave.vcd

.PHONY:all clean sim

all:
	@echo "No default target.Use 'make sim' to run the simulation."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	$(VERILATOR) $(VERILATOR_FLAGS)
	$(EXECUTABLE) &> /dev/null & PID=$$!; sleep 10;kill -9 $$PID
	@if ! command -v gtkwave &> /dev/null; then \
		echo "gtkwave not found.Installing gtkwave and dependencie..."; \
                sudo apt-get update && sudo apt-get install -y gtkwave libcanberra-gtk-module; \
        fi
	gtkwave $(WAVE_VCD); \
clean:
	rm -rf $(OBJ_DIR) $(WAVE_VCD) 

include ../Makefile
