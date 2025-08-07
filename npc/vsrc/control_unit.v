module control_unit(
	input [31:0] instruction,
	output [31:0] imm,
	output [4:0] rs1,
	output [4:0] rs2,
	output [4:0] rd,
	output       reg_write,//寄存器写使能信号
	output       alu_src,//0: rs2, 1: 立即数
	output [3:0] alu_ctrl,//选择 ALU 操作
	output       wb_src, //写回数据选择 (0: ALU结果, 1: 立即数)
	output       alu_enable, //ALU使能信号
	output       alu_r1, // AUIPC.JAL用PC，其他用rs1
	output       is_jal, // JAL 指令标志
	output       is_jalr, // JALR 指令标志
	output [2:0] b_type, // B-type 指令类型标志 
	output       is_b,    //B-type 指令标志 
	output [2:0] is_load, //Load 指令
	output [2:0] is_store //Store 指令
);
	wire [6:0] opcode = instruction[6:0];
	wire [2:0] funct3 = instruction[14:12];
	wire [6:0] funct7 = instruction[31:25];
	
   // 立即数生成（数据流）
   assign imm = (opcode == 7'b0110111) ? {instruction[31:12], 12'b0} :               // LUI
             (opcode == 7'b0010111) ? {instruction[31:12], 12'b0} :               // AUIPC
             (opcode == 7'b0010011) ? 
                 (funct3 == 3'b000) ? {{20{instruction[31]}}, instruction[31:20]} :  // ADDI
                 (funct3 == 3'b001) ? {{27{1'b0}}, instruction[24:20]} :           // SLLI（shamt[4:0]）
                 (funct3 == 3'b010) ? {{20{instruction[31]}}, instruction[31:20]} :  // SLTI
                 (funct3 == 3'b011) ? {{20{instruction[31]}}, instruction[31:20]} :  // SLTIU
                 (funct3 == 3'b100) ? {{20{instruction[31]}}, instruction[31:20]} :  // XORI
                 (funct3 == 3'b101) ? 
                     (funct7 == 7'b0000000) ? {{27{1'b0}}, instruction[24:20]} :     // SRLI
                     (funct7 == 7'b0100000) ? {{27{1'b0}}, instruction[24:20]} :     // SRAI
                     32'b0 :                                                        // 非法立即数
                 (funct3 == 3'b110) ? {{20{instruction[31]}}, instruction[31:20]} :  // ORI
                 (funct3 == 3'b111) ? {{20{instruction[31]}}, instruction[31:20]} :  // ANDI
                 32'b0 :                                                           // 其他I-type默认
             (opcode == 7'b1101111) ? {{11{instruction[31]}}, instruction[31], instruction[19:12], instruction[20], instruction[30:21], 1'b0} : // JAL
             (opcode == 7'b1100111) ? {{20{instruction[31]}}, instruction[31:20]} :     // JALR
             (opcode == 7'b1100011) ? {{20{instruction[31]}}, instruction[7], instruction[30:25], instruction[11:8], 1'b0 } :	//B-type
    	     (opcode == 7'b0000011) ? {{20{instruction[31]}}, instruction[31:20]} : //Load 指令
    	     (opcode == 7'b0100011) ? {{20{instruction[31]}}, instruction[31:25], instruction[11:7]} : //Store 指令
    	     32'b0;
             
    // 寄存器操作
    assign rs1 = instruction[19:15];
    assign rs2 = instruction[24:20];
    assign rd  = instruction[11:7];
    
    // 控制信号（数据流）
    assign reg_write = (opcode == 7'b0110111) |                          // LUI
                     (opcode == 7'b0010111) |                          // AUIPC
                     (opcode == 7'b0010011) |                          // ADDI,ANDI,ORI,XORI,STLI,STLIU,SLLI,SRLI,SRAI
                     (opcode == 7'b1101111) |                          // JAL
                     (opcode == 7'b1100111) |                          // JALR
                     (opcode == 7'b0110011) |				// ADD,SUB,AND,OR,XOR,STL,STLU,SLL,SRL,SRA
                     (opcode == 7'b0000011);
                     
    assign alu_src = (opcode == 7'b0010011) |   		    // ADDI,ANDI,ORI,XORI,STLI,STLIU,SLLI,SRLI,SRAI
    		   (opcode == 7'b1100111) |                          // JALR
    		   (opcode == 7'b1101111) |			     // JAL
    		   (opcode == 7'b0010111) |                           // AUIPC
    		   (opcode == 7'b0000011) |		             //LOAD 指令
    		   (opcode == 7'b0100011);			    //STORE指令

    assign alu_ctrl = 
     // R-type指令 (opcode=7'b0110011)
     (opcode == 7'b0110011) ? 
        (funct3 == 3'b000 && funct7 == 7'b0000000) ? 4'b0000 : // ADD
        (funct3 == 3'b000 && funct7 == 7'b0100000) ? 4'b0001 : // SUB
        (funct3 == 3'b111 && funct7 == 7'b0000000) ? 4'b0011 : // AND
        (funct3 == 3'b110 && funct7 == 7'b0000000) ? 4'b0100 : // OR
        (funct3 == 3'b100 && funct7 == 7'b0000000) ? 4'b0101 : // XOR
        (funct3 == 3'b010 && funct7 == 7'b0000000) ? 4'b0110 : // SLT
        (funct3 == 3'b011 && funct7 == 7'b0000000) ? 4'b0111 : // SLTU
        (funct3 == 3'b001 && funct7 == 7'b0000000) ? 4'b1000 : // SLL
        (funct3 == 3'b101 && funct7 == 7'b0000000) ? 4'b1001 : // SRL
        (funct3 == 3'b101 && funct7 == 7'b0100000) ? 4'b1010 : // SRA
        4'b0000 : // 默认
     // I-type指令 (opcode=7'b0010011)
     (opcode == 7'b0010011) ?
        (funct3 == 3'b000) ? 4'b0000 : // ADDI
        (funct3 == 3'b010) ? 4'b0110 : // SLTI
        (funct3 == 3'b011) ? 4'b0111 : // SLTIU
        (funct3 == 3'b100) ? 4'b0101 : // XORI
        (funct3 == 3'b110) ? 4'b0100 : // ORI
        (funct3 == 3'b111) ? 4'b0011 : // ANDI
        (funct3 == 3'b001 && funct7 == 7'b0000000) ? 4'b1000 : // SLLI
        (funct3 == 3'b101 && funct7 == 7'b0000000) ? 4'b1001 : // SRLI
        (funct3 == 3'b101 && funct7 == 7'b0100000) ? 4'b1010 : // SRAI
        4'b0000 : // 默认
    // B-type指令 (opcode=7'b1100011)
    (opcode == 7'b1100011) ?
    	(funct3 == 3'b000) ? 4'b1011 : // BEQ
    	(funct3 == 3'b001) ? 4'b1011 : // BNE
    	(funct3 == 3'b100) ? 4'b0110 : // BLT
    	(funct3 == 3'b101) ? 4'b0110 : // BGE
    	(funct3 == 3'b110) ? 4'b0111 : // BLTU
    	(funct3 == 3'b111) ? 4'b0111 : // BGEU
        4'b0000 :
    //Load 指令
    (opcode == 7'b0000011) ?
    	4'b0000 :
    //Store 指令
    (opcode == 7'b0100011) ?
    	4'b0000 :
    4'b0000;
    
    assign wb_src = (opcode == 7'b0110111);
     	
    assign alu_enable = (opcode != 7'b0110111) & //LUI不需要alu
    			(opcode != 7'b1101111) & //JAL不需要alu
    			(opcode != 7'b1100111);  //JALR不需要alu
    
    assign alu_r1 = (opcode == 7'b0010111); // AUIPC用PC
    
    assign is_jal = (opcode == 7'b1101111); 
    
    assign is_jalr = (opcode == 7'b1100111);
    
    assign b_type = 
      (opcode == 7'b1100011) ?
    	(funct3 == 3'b000) ? 3'b001 : // BEQ
    	(funct3 == 3'b001) ? 3'b010 : // BNE
    	(funct3 == 3'b100) ? 3'b011 : // BLT
    	(funct3 == 3'b101) ? 3'b100 : // BGE
    	(funct3 == 3'b110) ? 3'b101 : // BLTU
    	(funct3 == 3'b111) ? 3'b110 : // BGEU
        3'b000 :
      3'b000;
      
    assign is_b = (opcode == 7'b1100011);
    
    assign is_load = (opcode == 7'b0000011) ? funct3 : 3'b111;
    
    assign is_store = (opcode == 7'b0100011) ? funct3 : 3'b111;
endmodule	
