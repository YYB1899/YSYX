module pc (
    input wire        clk,
    input wire        rst,
    output reg [31:0] pc,
    input [31:0]      imm,
    input [31:0]      sum,
    input [2:0]       b_type,
    input             is_b,
    input [31:0]      rs1_data,
    input wire        is_jal,
    input wire        is_jalr,
    output wire [31:0] pc_jal
);
    // 单周期处理器不需要next_pc寄存器
    wire [31:0] next_pc;
    
    // 添加调试信息
    always @(posedge clk) begin
        if (!rst) begin
            if (is_jalr) begin
                $display("JALR DEBUG: PC=%h, rs1_data=%h, imm=%h, target=%h", 
                         pc, rs1_data, imm, (rs1_data + imm) & ~32'b1);
            end
            if (is_jal) begin
                $display("JAL DEBUG: PC=%h, imm=%h, target=%h", 
                         pc, imm, pc + imm);
            end
        end
    end
    
    // 组合逻辑计算下一条PC
assign next_pc = 
    is_jalr ? (rs1_data + imm) & ~32'b1 :  // JALR
    (is_jal ||   
     (is_b && 
      ((b_type == 3'b001 && sum == 32'b0) ||  // BEQ
       (b_type == 3'b010 && sum != 32'b0) ||  // BNE
       (b_type == 3'b011 && sum == 32'b1) ||  // BLT
       (b_type == 3'b100 && sum == 32'b0) ||  // BGE
       (b_type == 3'b101 && sum == 32'b1) ||  // BLTU
       (b_type == 3'b110 && sum == 32'b0))))  // BGEU
    ? pc + imm : pc + 32'h00000004;
  
assign pc_jal = pc + 32'h00000004;
  
    // 时序逻辑更新PC
    always @(posedge clk or posedge rst) begin
        if (rst) pc <= 32'h80000000;
        else pc <= next_pc;
    end

endmodule
