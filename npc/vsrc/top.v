module top (
    input  wire        clk,          // 时钟信号
    input  wire        rst,          // 复位信号
    output wire [31:0] pc,           // 程序计数器
    output wire [31:0] instruction,  // 当前指令
    output wire        overflow     // ALU 溢出信号
);

    // 内部信号
    wire [4:0]  rs1, rs2, rd;       // 寄存器地址
    wire [31:0] rs1_data, rs2_data, rd_data; // 寄存器数据
    wire        reg_write;          // 寄存器写使能
    wire        alu_src;            // ALU 操作数选择
    wire [3:0]  alu_ctrl;           // ALU 控制信号
    wire [31:0] alu_result;         // ALU 计算结果
    wire [31:0] imm;                // 符号扩展后的立即数
    wire        wb_src;             // 写回数据选择 (0: ALU结果, 1: 立即数)
    wire        alu_enable;         // alu使能信号
    wire        alu_r1;             // AUIPC用PC，其他用rs1
    wire        is_jal;   	    // JAL 指令标志
    wire        is_jalr; 	    // JALR 指令标志
    wire [31:0] pc_jal;	            // 输出JAL的PC+4值
    wire [2:0]  b_type;		    // B-type 指令类型标志 
    wire        is_b;		    // B-type 指令志 
    wire [2:0]  is_load;	    //Load 指令
    wire [2:0]  is_store;	    //Store 指令
    wire        use_wdata;
    
    // 实例化 PC 模块	
    pc pc_inst (
        .clk         (clk),
        .rst         (rst),
        .pc          (pc),
        .is_jalr     (is_jalr),
        .is_jal      (is_jal),
        .imm         (imm),
        .rs1_data    (rs1_data),
        .pc_jal      (pc_jal),
        .b_type      (b_type),
        .is_b        (is_b),
        .sum         (alu_result)
    );

    // 实例化 IMEM 模块
    imem imem_inst (
        .pc         (pc),
        .instruction(instruction)
    );
    
    //ebreak结束仿真
    ebreak_detector ebreak_detector_inst (
        .clk         (clk),
        .rst         (rst),
        .pc          (pc),
        .instruction(instruction)
    ); 
    
    // 实例化 Control Unit 模块
    control_unit control_unit_inst (
        .instruction (instruction),
        .imm         (imm),
        .rs1         (rs1),
        .rs2         (rs2),
        .rd          (rd),
        .reg_write   (reg_write),
        .alu_src     (alu_src),
        .alu_ctrl    (alu_ctrl),
        .wb_src      (wb_src),
        .alu_enable  (alu_enable),
        .alu_r1      (alu_r1),
        .is_jalr     (is_jalr),
        .is_jal      (is_jal),
        .b_type      (b_type),
        .is_b        (is_b),
        .is_load     (is_load),
        .is_store    (is_store)
    );

    // 实例化 Register File 模块
    register_file register_file_inst (
        .clk        (clk),
        .rs1        (rs1),
        .rs2        (rs2),
        .rd         (rd), 
        .wen        (reg_write),
        .wdata      ((use_wdata) ? rd_data : ((is_jal || is_jalr) ? pc_jal : (wb_src ? imm : alu_result))), 
        .rs1_data   (rs1_data),
        .rs2_data   (rs2_data)
    );

    // 实例化 ALU 模块
    alu alu_inst (
        .r1         (alu_r1 ? pc : rs1_data),
        .r2         (alu_src ? imm : rs2_data), // 选择立即数或 rs2_data
        .sub        (alu_ctrl), // 仅支持加法，SUB 固定为 0
        .sum        (alu_result),
        .overflow   (overflow),
        .alu_enable (alu_enable)
    );
    
    trap trap(
    	.clk(clk),
    	.rst(rst),
    	.pc(pc),
    	.instruction(instruction),
    	.overflow(overflow)
    );

    memory_interface memory_interface(
	.clk         (clk),
	.rst         (rst),
	.alu_result  (alu_result),
        .is_load     (is_load),
        .is_store    (is_store),
	.wdata       (rs2_data),
	.rdata       (rd_data),
	.use_wdata   (use_wdata)
    );
endmodule
