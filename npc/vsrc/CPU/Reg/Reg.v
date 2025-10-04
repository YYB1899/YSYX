// 触发器模板
module Reg #(WIDTH = 1, RESET_VAL = 0) ( //默认寄存器宽度和复位值
    input                        clock                      ,
    input                        reset                      ,
    input              [WIDTH-1: 0]din                      , //in
    output reg         [WIDTH-1: 0]dout                     , //out
    input                        wen                          //使能
);
always @(posedge clock) begin
if (reset) dout <= RESET_VAL; //复位
else if (wen) dout <= din;    //写入
end
endmodule

