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
            if (instruction == 32'b000000000001_00000_000_00000_1110011) begin
                 $display("\n=================================");
            	$display(`GREEN,"  HIT GOOD TRAP (^_^)",`RESET);
            	$display(`BOLD,"  PC: %h",`RESET, pc);
            	$display("=================================\n");
                $finish(0);
            end
            // BAD TRAP 检测（非法指令或溢出）
            else if (overflow || (instruction[6:0] == 7'b1111111)) begin
                 $display("\n=================================");
            	$display(`RED,"  HIT BAD TRAP (X_X)",`RESET);
            	$display(`BOLD,"  PC:        %h",`RESET, pc);
            	$display("=================================\n");
                $finish(1);
            end
        end
    end
endmodule
