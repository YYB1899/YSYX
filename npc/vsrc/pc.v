module pc (
	input wire clk,
	input wire rst,
	output reg [31:0] pc,
	input [31:0] imm,
	input [31:0] rs1_data,
	input wire is_jal,
	input wire is_jalr,
	output wire [31:0] pc_jal
);
    reg [31:0] next_pc;
    wire [31:0] jal_pc_plus4 = pc + 4;  // 计算jal的PC+4

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            pc <= 32'h80000000; // 复位时初始PC
            next_pc <= 32'h80000000;
        end else begin
            // 计算下一条PC值
            if (is_jalr) begin
                next_pc <= (rs1_data + imm) & ~32'b1;  // JALR: (rs1 + imm) & ~1
            end else if (is_jal) begin
                next_pc <= pc + imm;                  // JAL: PC + 立即数偏移
            end else begin
                next_pc <= pc + 4;                    // 默认顺序执行
            end
            
            // 更新PC寄存器
            pc <= next_pc;
    end

    // 输出jal的PC+4值
    assign pc_jal = jal_pc_plus4;
    end
endmodule
