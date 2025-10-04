module sext#
(
    parameter DATA_WIDTH=1, //输入位宽
    parameter OUT_WIDTH=2   //输出位宽
)
(
    input [DATA_WIDTH-1:0]data,
    output [OUT_WIDTH-1:0]sext_data
);

assign sext_data = {{(OUT_WIDTH-DATA_WIDTH){data[DATA_WIDTH-1]}},data};  //{(OUT_WIDTH-DATA_WIDTH){data[DATA_WIDTH-1]}} - 复制符号位；{扩展位, data} - 位拼接操作



endmodule

