module ebreak_detector (
    input wire clk,
    input wire rst,
    input wire [31:0] pc,
    input wire [31:0] instruction
);
    import "DPI-C" function void end_simulation();

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            // 复位时不操作
        end else begin
            if (instruction == 32'b000000000001_00000_000_00000_1110011) begin
                $display("Detected ebreak instruction at PC = %h", pc);
                end_simulation();  // 调用DPI-C函数结束仿真
            end
        end
    end
endmodule
