module pc (
	input wire clk,
	input wire rst,
	output reg [31:0] pc
);
	initial begin
		pc = 32'h80000000;
	end
	
	always @(posedge clk or posedge rst) begin
		if(rst) begin
			pc <= 32'h80000000;
		end else begin
			pc <= pc + 4;
		end
	end
endmodule
