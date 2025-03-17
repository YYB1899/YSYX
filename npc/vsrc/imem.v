module imem (
    input  [31:0] pc,           // 程序计数器输入
    output reg [31:0] instruction // 32 位指令输出
);
    reg [31:0] rom[255:0];
    initial begin 
        $readmemb("/home/yyb/ysyx-workbench/npc/instruction.txt",rom);
        for (integer i = 4; i < 256; i = i + 1) begin
           rom[i] = 32'b0; 
        end   
    end
    assign instruction = rom[(pc - 32'h80000000)>>2];
endmodule
