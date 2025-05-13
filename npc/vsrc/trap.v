module trap (
	input wire clk,
	input wire rst,
	output reg [31:0] pc,
	input wire [31:0] instruction,
	input wire overflow
);

`define RED   "\033[1;31m"
`define GREEN "\033[1;32m"
`define BOLD  "\033[1m"
`define RESET "\033[0m"

always @(posedge clk) begin
    if (!rst) begin
        // GOOD TRAP 检测（ebreak）
        if (instruction == 32'h00100073) begin
            $display("\n=================================");
            $display(`GREEN,"  HIT GOOD TRAP (^_^)",`RESET);
            $display("  PC: %h", pc);
            $display("  Instruction: %h", instruction);
            $display("=================================\n");
            $finish(0);
        end
        // BAD TRAP 检测（非法指令）
        else if (is_illegal_instruction(instruction)) begin
            $display("\n=================================");
            $display(`RED,"  HIT BAD TRAP - ILLEGAL INSTRUCTION (X_X)",`RESET);
            $display("  PC: %h", pc);
            $display("  Invalid Instruction: %h", instruction);
            $display("=================================\n");
            $finish(1);
        end
    end
end

// 非法指令检测函数
function automatic is_illegal_instruction;
    input [31:0] instr;
    reg [6:0] opcode;
    reg [2:0] funct3;
    reg [6:0] funct7;
    begin
        opcode = instr[6:0];
        funct3 = instr[14:12];
        funct7 = instr[31:25];
        
        // 检查已知合法指令
        case(opcode)
            // 标准RV32I指令
            7'b0110111, // LUI
            7'b0010111, // AUIPC
            7'b1101111, // JAL
            7'b1100111, // JALR
            7'b1100011, // 分支指令
            7'b0000011, // 加载指令
            7'b0100011, // 存储指令
            7'b0010011, // 立即数运算
            7'b0110011, // 寄存器运算
            7'b0001111, // FENCE
            7'b1110011: // ECALL/EBREAK
                is_illegal_instruction = 0;
            default:
                is_illegal_instruction = 1; // 未知操作码
        endcase

    end
endfunction
endmodule
