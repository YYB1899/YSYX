module trap (
	input wire clk,
	input wire rst,
	input wire [31:0] pc,
	input wire [31:0] instruction,
	input wire overflow
);

`define RED   "\033[1;31m"
`define GREEN "\033[1;32m"
`define BOLD  "\033[1m"
`define RESET "\033[0m"

// 添加计数器来控制调试输出频率
reg [31:0] debug_counter = 0;

always @(posedge clk) begin
    if (!rst) begin
        debug_counter <= debug_counter + 1;
        
        // 每10000个周期打印一次PC，帮助调试
        if (debug_counter % 10000 == 0) begin
            $display("DEBUG: Cycle %d, PC = %h, Instruction = %h", debug_counter, pc, instruction);
        end
        
        // 特别关注接近目标地址的PC
        if (pc >= 32'h80000130 && pc <= 32'h80000150) begin
            $display("NEAR TARGET: PC = %h, Instruction = %h", pc, instruction);
        end
        
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
            7'b0010011, // ADDI,ANDI,ORI,XORI,STLI,STLIU,SLLI,SRLI,SRAI
            7'b0110011, // ADD,SUB,AND,OR,XOR,STL,STLU
            7'b0100011, // SW
            7'b0001111, // FENCE
            7'b1110011, // ECALL/EBREAK
            7'b0000011,
            7'b1100011:
                is_illegal_instruction = 0;
            default:
                is_illegal_instruction = 1; // 未知操作码
        endcase

    end
endfunction
endmodule
