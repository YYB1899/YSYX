module imem (
    input  wire [31:0] pc,           // 程序计数器输入
    output wire [31:0] instruction   // 32 位指令输出
);

    // 指令存储器（8KB，对应RISC-V的32位地址按字寻址）
    reg [31:0] rom [0:2047];  // 2048 words = 8192 bytes

    // 从ELF文件加载指令（自动提取.text段）
    initial begin
        integer i;
        // 初始化全部为0（nop）
        for (i = 0; i < 2048; i = i + 1) begin
            rom[i] = 32'h00000013;  // nop
        end
        
        // 使用readmemh加载处理后的指令文件
        $readmemh("build/inst.hex", rom);
     end   
  // 地址转换（按字寻址，支持80000000起始地址）
    wire [31:0] word_addr = (pc - 32'h80000000) >> 2;
    
    // 输出指令（地址越界时返回nop）
    assign instruction = (word_addr < 2048) ? rom[word_addr] : 32'h00000013;
endmodule
