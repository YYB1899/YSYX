module imem (
    input  [31:0] pc,           // 程序计数器输入
    output reg [31:0] instruction // 32 位指令输出
);
    reg [31:0] rom[255:0];
    //initial begin 
      //   $readmemb("file.txt",rom);
    //end
    
    initial begin
        rom[0] = 32'b000000000101_00010_000_00001_0010011; // addi x1, x2, 5
        rom[1] = 32'b000000001010_00011_000_00010_0010011; // addi x2, x3, 10
        rom[2] = 32'b000000001111_00100_000_00011_0010011; // addi x3, x4, 15
        rom[3] = 32'b000000000001_00000_000_00000_1110011; // ebreak
        for (integer i = 4; i < 256; i = i + 1) begin
            rom[i] = 32'b0; 
        end
    end

    assign instruction = rom[(pc - 32'h80000000)>>2];
endmodule
