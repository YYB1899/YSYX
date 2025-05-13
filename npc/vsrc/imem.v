module imem (
    input  wire [31:0] pc,
    output wire [31:0] instruction  // 改回组合逻辑输出
);
    reg [7:0] rom [0:8191];
    wire [31:0] word_addr = (pc - 32'h80000000);  // 字节地址

    // 组合逻辑计算当前指令
    assign instruction = {rom[word_addr+3], rom[word_addr+2], 
                                   rom[word_addr+1], rom[word_addr]};

    initial begin
        $readmemh("build/inst.hex", rom);
        $display("===== Instruction Memory Contents =====");
        for (int i = 0; i < 12; i++) begin
            $display("imem[%h] = %h", 32'h80000000 + i*4, 
                    {rom[i*4+3], rom[i*4+2], rom[i*4+1], rom[i*4]});
        end
    end
endmodule
