module imem (
    input  wire [31:0] pc,
    output wire [31:0] instruction
);
    reg [7:0] rom [0:8191];  // 字节数组
    wire [31:0] word_addr = (pc - 32'h80000000);  // 字节地址

    // 从字节数组组合为字（按小端）
    assign instruction = {rom[word_addr+3], rom[word_addr+2], 
                         rom[word_addr+1], rom[word_addr]};

    initial begin
        $readmemh("build/inst.hex", rom);  // 直接加载字节流
        $display("===== Correct Instruction Memory =====");
        for (int i = 0; i < 12; i++) begin
            $display("imem[%h] = %h", 32'h80000000 + i*4, 
                    {rom[i*4+3], rom[i*4+2], rom[i*4+1], rom[i*4]});
        end
    end
endmodule
