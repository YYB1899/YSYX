`include "../define/para.v"
module RegisterFile #(ADDR_WIDTH = 32, DATA_WIDTH = 5) ( //地址宽度和数据宽度
    input                        clock                        ,
    input              [DATA_WIDTH-1: 0]wdata               , //写数据
    input              [ADDR_WIDTH-1: 0]waddr               , //写地址
    input                        wen                        , //写使能
    input                        reset                      ,
    input              [ADDR_WIDTH-1: 0]rs1_addr            , //读地址
    input              [ADDR_WIDTH-1: 0]rs2_addr            ,

    output             [DATA_WIDTH-1: 0]rs1_value           , //读数据
    output             [DATA_WIDTH-1: 0]rs2_value           , 
    output             [DATA_WIDTH-1: 0]a0_value              //寄存器a0值

);
    reg                [DATA_WIDTH-1: 0]rf        [2**ADDR_WIDTH-1:0]  ; //5位宽，2^32个的寄存器组
  always @(posedge clock) begin //写操作
    if (wen) rf[waddr] <= wdata;
  end

  always @(posedge clock)begin //寄存器a0始终为0
     if(reset)
      rf[0] <= 0;
  end

`ifdef Performance_Count
task ReadReg;
    input                        int reg_num                ;
    output                       bit[31:0] reg_value        ;
    reg_value = rf[reg_num];
endtask

export "DPI-C" task ReadReg; //DPI-C机制确保CPP读取寄存器值
`endif 

    assign                       rs1_value                 = rf[rs1_addr]; //读操作
    assign                       rs2_value                 = rf[rs2_addr];
    assign                       a0_value                  = rf[10];


endmodule

