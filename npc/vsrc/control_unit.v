module control_unit(
	input [31:0] instruction,
	output [11:0] imm,
	output [4:0] rs1,
	output [4:0] rs2,
	output [4:0] rd,
	output       reg_write,//寄存器写使能信号
	output       alu_src,//0: rs2, 1: 立即数
	output [2:0] alu_ctrl//选择 ALU 操作
);
	wire [6:0] opcode = instruction[6:0];
	wire [2:0] funct = instruction[14:12];
	wire [6:0] funct_r = instruction[31:25];
		
	assign imm = instruction[31:20];
	assign rs1 = instruction[19:15];
	assign rs2 = instruction[24:20];
	assign rd = instruction[11:7];
	
	assign reg_write = (opcode == 7'b0010011) && (funct == 3'b000);
	assign alu_src = (opcode == 7'b0010011) && (funct == 3'b000);
	assign alu_ctrl = 3'b000;

endmodule	
