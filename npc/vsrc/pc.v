module pc (
    input wire        clk,
    input wire        rst,
    output reg [31:0] pc_out,
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
    
    // 分支条件判断
    wire branch_taken;
    assign branch_taken = is_b && 
      ((b_type == 3'b001 && sum == 32'b1) ||  // BEQ - 相等时跳转
       (b_type == 3'b010 && sum == 32'b0) ||  // BNE - 不相等时跳转
       (b_type == 3'b011 && sum == 32'b1) ||  // BLT
       (b_type == 3'b100 && sum == 32'b0) ||  // BGE
       (b_type == 3'b101 && sum == 32'b1) ||  // BLTU
       (b_type == 3'b110 && sum == 32'b0));   // BGEU
    
    // 添加调试信息
    always @(posedge clk) begin
        if (!rst) begin
            if (is_jalr) begin
                $display("JALR DEBUG: PC=%h, rs1_data=%h, imm=%h, target=%h", 
                         pc_out, rs1_data, imm, (rs1_data + imm) & ~32'b1);
            end
            if (is_jal) begin
                $display("JAL DEBUG: PC=%h, imm=%h, target=%h", 
                         pc_out, imm, pc_out + imm);
            end
            if (is_b) begin
                $display("BRANCH DEBUG: PC=%h, b_type=%b, sum=%h, branch_taken=%b", 
                         pc_out, b_type, sum, branch_taken);
                $display("  Target=%h, Next_PC=%h", pc_out + imm, next_pc);
                case (b_type)
                    3'b001: $display("  BEQ: sum==1? %b (相等时跳转)", sum == 32'b1);
                    3'b010: $display("  BNE: sum==0? %b (不相等时跳转)", sum == 32'b0);
                    3'b011: $display("  BLT: sum==1? %b", sum == 32'b1);
                    3'b100: $display("  BGE: sum==0? %b", sum == 32'b0);
                    3'b101: $display("  BLTU: sum==1? %b", sum == 32'b1);
                    3'b110: $display("  BGEU: sum==0? %b", sum == 32'b0);
                    default: $display("  Unknown branch type");
                endcase
            end
        end
    end
    
    // 组合逻辑计算下一条PC
assign next_pc = 
    is_jalr ? (rs1_data + imm) & ~32'b1 :  // JALR
    (is_jal || branch_taken) ? pc_out + imm : pc_out + 32'h00000004;
  
assign pc_jal = pc_out + 32'h00000004;
  
    // 时序逻辑更新PC
    always @(posedge clk or posedge rst) begin
        if (rst) pc_out <= 32'h80000000;
        else pc_out <= next_pc;
    end

endmodule
