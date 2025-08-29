module imem (
    input  wire [31:0] pc,               // 输入指令地址
    output wire [31:0] instruction       // 输出32位指令
);
    reg [31:0] inst_reg;
    // DPI-C 接口声明
    import "DPI-C" function int pmem_read(
        input int raddr
    );
    // 组合逻辑读取指令
    always @(*) begin
        inst_reg = pmem_read(pc);        // 调用C函数读取指令
    end   
    assign instruction = inst_reg;       // 输出指令
endmodule
