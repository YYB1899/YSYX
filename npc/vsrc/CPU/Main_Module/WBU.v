/* verilator lint_off UNUSEDSIGNAL */
// signal not use
`include "../define/para.v"
module WBU (
    input                               clock                      ,
    input                               reset                      ,

    input              [  31: 0]        MEM_Rdata                  , //内存读取数据
    input              [  31: 0]        Ex_result                  ,
    input              [  31: 0]        rd_value                   , //目标寄存器的写回值
    input              [   4: 0]        rd                         ,
    input              [   3: 0]        csr_wen                    ,
    input                               R_wen                      ,
    input                               mem_ren                    ,
    input                               jump_flag                  ,


    input                               valid                      , //LSU的valid
    output                              ready                      , //WBU的ready

    output                              valid_next                 , //WBU的valid
    output                              R_wen_next                 ,
    output             [   3: 0]        csr_wen_next               , //CSR寄存器写使能
    output             [  31: 0]        csrd                       , //CSR寄存器写入的数据
`ifdef Performance_Count
    input              [  31: 0]        pc                         ,
    output             [  31: 0]        pc_next                    ,
    input                               mem_wen_reg                , //内存写使能寄存器
    output                              mem_wen_flag               , //内存写标志
    output                              mem_ren_flag               , //内存读标志
    output             [  31: 0]        paddr                      , //写回的物理地址
    input              [  31: 0]        inst                       ,
    output             [  31: 0]        inst_next                  ,
`endif
    output             [  31: 0]        rd_value_next              , //写回寄存器的数据
    output             [   4: 0]        rd_next                      //写回寄存器的地址
);


`ifdef Performance_Count
    assign                              mem_ren_flag                = mem_ren;
    assign                              paddr                       = Ex_result;
    assign                              mem_wen_flag                = mem_wen_reg;
    assign                              pc_next                     = pc;
    assign                              inst_next                   = inst;
`endif

    assign                              valid_next                  = valid;
    assign                              rd_value_next               = (jump_flag | (|csr_wen) )? //跳转指令或CSR优先
                                                                        rd_value : (mem_ren)   ? //加载指令其次
                                                                        MEM_Rdata: Ex_result;    //正常情况最后
    assign                              csrd                        = Ex_result;
    assign                              csr_wen_next                = csr_wen;
    assign                              R_wen_next                  = R_wen&valid;
    assign                              rd_next                     = rd;
    assign                              ready                       = 1'b1;

endmodule                                                           //WBU
