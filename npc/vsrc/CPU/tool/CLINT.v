`timescale 1ns / 1ps
module CLINT
(
    input                               reset                      ,
    input                               clock                      ,

    input              [32-1: 0]        araddr                     ,
    input                               arvalid                    ,
    output reg                          arready                    ,
    input              [   3: 0]        arid                       ,
    input              [   7: 0]        arlen                      ,
    input              [   2: 0]        arsize                     ,
    input              [   1: 0]        arburst                    ,

    input                               rready                     ,
    output reg         [32-1: 0]        rdata                      ,
    output reg         [   1: 0]        rresp                      ,
    output reg                          rvalid                     ,
    output reg                          rlast                      ,
    output reg         [   3: 0]        rid                        ,

    input              [32-1: 0]        awaddr                     ,
    input                               awvalid                    ,
    output                              awready                    ,
    input              [   3: 0]        awid                       ,
    input              [   7: 0]        awlen                      ,
    input              [   2: 0]        awsize                     ,
    input              [   1: 0]        awburst                    ,

    input              [32-1: 0]        wdata                      ,
    input              [   3: 0]        wstrb                      ,
    input                               wvalid                     ,
    output                              wready                     ,
    input                               wlast                      ,
    
    output             [   1: 0]        bresp                      ,
    output                              bvalid                     ,
    input                               bready                     ,
    output             [   3: 0]        bid                         
);
    reg                [  31: 0]        clk_count_low               ;
    reg                [  31: 0]        clk_count_high              ;
    //高低组成mtime寄存器
    reg                                 state                       ;
    localparam                          IDLE                       = 1'b0  ;
    localparam                          ARRE                       = 1'b1  ;

    always @(posedge clock or posedge reset) begin
        if(reset)
            state <= IDLE;
        else begin
            case(state)
            IDLE:state <= (arready & arvalid)? ARRE : IDLE;
            ARRE:state <= (rvalid & rready)? IDLE : ARRE;
            endcase
        end
    end
    //状态机逻辑控制

    always @(posedge clock) begin
        if(reset)
            clk_count_low <= 0;
        else if(clk_count_low == ~32'h0)
            clk_count_low <= 0;
        else
            clk_count_low <= clk_count_low + 1;
    end
    //计数器低32位:为0则自增；为1则归0
    always @(posedge clock) begin
        if(reset)
            clk_count_high <= 0;
        else if(clk_count_low == ~32'h0)
            clk_count_high <= clk_count_high + 1;
        else
            clk_count_high <= clk_count_high;
    end
    //计数器高32位：为0则不变；为1则进位
    assign                              arready                     = state == IDLE;
    assign                              rresp                       = 0;
    assign                              rvalid                      = (state == ARRE);
    assign                              rlast                       = rvalid;
    assign                              rdata                       = (araddr == 32'h2000004)            ?    //mtime高32位
                                                                      clk_count_high:(araddr == 32'h2000000)? //mtime低32位
                                                                      clk_count_low:0;

endmodule


