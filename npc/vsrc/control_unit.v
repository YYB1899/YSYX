module control_unit(
	input [31:0] instruction,
	output [31:0] imm,
	output [4:0] rs1,
	output [4:0] rs2,
	output [4:0] rd,
	output       reg_write,//寄存器写使能信号
	output       alu_src,//0: rs2, 1: 立即数
	output [2:0] alu_ctrl,//选择 ALU 操作
	output       wb_src, //写回数据选择 (0: ALU结果, 1: 立即数)
	output       alu_enable, //ALU使能信号
	output       alu_r1, // AUIPC.JAL用PC，其他用rs1
	output       is_jal, // JAL 指令标志
	output       is_jalr // JALR 指令标志
);
	wire [6:0] opcode = instruction[6:0];
	wire [2:0] funct = instruction[14:12];
	wire [6:0] funct_r = instruction[31:25];
  // 立即数生成（数据流）
    assign imm = (opcode == 7'b0110111) ? {instruction[31:12], 12'b0} :               // LUI
                (opcode == 7'b0010111) ? {instruction[31:12], 12'b0} :               // AUIPC
                (opcode == 7'b0010011) ? {{20{instruction[31]}}, instruction[31:20]} :     // ADDI
                (opcode == 7'b1101111) ? {{11{instruction[31]}}, instruction[31], instruction[19:12], instruction[20], instruction[30:21], 1'b0} : // JAL
                (opcode == 7'b1100111) ? {{20{instruction[31]}}, instruction[31:20]} :     // JALR
                32'b0;
    // 寄存器操作
    assign rs1 = instruction[19:15];
    assign rs2 = instruction[24:20];
    assign rd  = instruction[11:7];
    
    // 控制信号（数据流）
    assign reg_write = (opcode == 7'b0110111) |                          // LUI
                     (opcode == 7'b0010111) |                          // AUIPC
                     (opcode == 7'b0010011) |                          // ADDI
                     (opcode == 7'b1101111) |                          // JAL
                     (opcode == 7'b1100111) |                           // JALR
                     (opcode == 7'b0110011);				// ADD,SUB

    
    assign alu_src = (opcode == 7'b0010011) |   		    // ADDI
    		   (opcode == 7'b1100111) |                          // JALR
    		   (opcode == 7'b1101111) |			     // JAL
    		   (opcode == 7'b0010111);                           // AUIPC
    
    assign alu_ctrl = (opcode == 7'b0110011 && funct_r == 7'b0100000) ? 3'b001 : 3'b000;                                          // 默认加法
    
    assign wb_src = (opcode == 7'b0110111);
     	
    assign alu_enable = (opcode != 7'b0110111) | //LUI不需要alu
    			(opcode != 7'b1101111) | //JAL不需要alu
    			(opcode == 7'b1100111);  //JALR不需要alu
    
    assign alu_r1 = (opcode == 7'b0010111); // AUIPC用PC
    assign is_jal = (opcode == 7'b1101111); 
    assign is_jalr = (opcode == 7'b1100111);
endmodule	
