module top (
    input  wire        clk,          // 时钟信号
    input  wire        rst,          // 复位信号
    output wire [31:0] pc,           // 程序计数器
    output wire [31:0] instruction,  // 当前指令
    output wire        overflow      // ALU 溢出信号
);

    // 内部信号
    wire [31:0] imm_extended;       // 符号扩展后的立即数
    wire [4:0]  rs1, rs2, rd;       // 寄存器地址
    wire [31:0] rs1_data, rs2_data; // 寄存器数据
    wire        reg_write;          // 寄存器写使能
    wire        alu_src;            // ALU 操作数选择
    wire [2:0]  alu_ctrl;           // ALU 控制信号
    wire [31:0] alu_result;         // ALU 计算结果
    wire [11:0] imm;


    // 实例化 PC 模块
    pc pc_inst (
        .clk         (clk),
        .rst         (rst),
        .pc          (pc)
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
        .alu_ctrl    (alu_ctrl)
    );

    // 实例化 SignExtend 模块
    sign_extend sign_extend_inst (
        .imm         (imm),
        .imm_extended(imm_extended)
    );

    // 实例化 Register File 模块
    register_file register_file_inst (
        .clk        (clk),
        .rs1        (rs1),
        .rs2        (rs2),
        .rd         (rd),
        .wen        (reg_write),
        .wdata      (alu_result),
        .rs1_data   (rs1_data),
        .rs2_data   (rs2_data)
    );

    // 实例化 ALU 模块
    alu alu_inst (
        .r1         (rs1_data),
        .r2         (alu_src ? imm_extended : rs2_data), // 选择立即数或 rs2_data
        .sub        (alu_ctrl), // 仅支持加法，SUB 固定为 0
        .sum        (alu_result),
        .overflow   (overflow)
    );
    
    trap trap(
    	.clk(clk),
    	.rst(rst),
    	.pc(pc),
    	.instruction(instruction),
    	.overflow(overflow)
    );
endmodule
