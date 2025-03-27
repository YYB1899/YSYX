module trap (
    input wire clk,
    input wire rst,
    input wire [31:0] pc,
    input wire [31:0] instruction,
    input wire overflow,
    input wire program_finished  // 新增：标记程序正常执行完毕
);

`define RED   "\033[1;31m"
`define GREEN "\033[1;32m"
`define BOLD  "\033[1m"
`define RESET "\033[0m"

    reg [31:0] last_pc;
    reg [31:0] last_instruction;
    reg last_overflow;

    // 记录最后状态
    always @(posedge clk) begin
        if (!rst) begin
            last_pc <= pc;
            last_instruction <= instruction;
            last_overflow <= overflow;
        end
    end

    // 陷阱检测
    always @(posedge clk) begin
        if (!rst) begin
            // GOOD TRAP - 程序正常执行完毕
            if (program_finished) begin
                $display("\n==========================================");
                $display(`GREEN,"  HIT GOOD TRAP (^_^)",`RESET);
                $display("  Last PC:        %h", last_pc);
                $display("  Last Instruction: %h", last_instruction);
                $display("==========================================\n");
                $finish(0);
            end
            // BAD TRAP - 检测到崩溃条件
            else  begin
                $display("\n==========================================");
                $display(`RED,"  HIT BAD TRAP (X_X)",`RESET);
                $display("  Fault PC:       %h", last_pc);
                $display("  Fault Instruction: %h", last_instruction);
                $display("  Overflow:       %b", last_overflow);
                $display("  Current State:");
                $display("    PC:           %h", pc);
                $display("    Instruction:   %h", instruction);
                $display("==========================================\n");
                $finish(1);
            end
        end
    end
endmodule
