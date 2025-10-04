module CSR #(
    parameter                           CSR_WIDTH                  = 32    , 
    parameter                           RESET_VAL                  = 0     
) // CSR寄存器位宽32,复位值0
(
    input                                  clock                      ,
    input                                  reset                      ,
    input              [  31: 0]           pc                         ,
    input                                  ecall_flag                 , //ecall标志
    input              [  31: 0]           csrd                       , //写数据
    input              [   3: 0]           csr_wen                    , //写使能（4个CSR）

    output             [  31: 0]           mvendorid_out              , //各个CSR的输出
    output             [  31: 0]           marchid_out                ,
    output             [  31: 0]           mepc_out                   ,
    output             [  31: 0]           mcause_out                 ,
    output             [  31: 0]           mstatus_out                ,
    output             [  31: 0]           mtvec_out                   
);


    wire               [  31: 0]        mepc_in                     ; //各个CSR的输入
    wire               [  31: 0]        mcause_in                   ;
    wire               [  31: 0]        mstatus_in                  ;
    wire               [  31: 0]        mtvec_in                    ;

    assign                              mepc_in                     = (ecall_flag)? pc : csrd;
    assign                              mcause_in                   = (ecall_flag)? 11 : csrd;
    assign                              mstatus_in                  = csrd;
    assign                              mtvec_in                    = csrd;
    assign                              mvendorid_out               = 32'h79737978;
    assign                              marchid_out                 = 32'h16FBCBD;

Reg #( //mepc寄存器
    .WIDTH                              (CSR_WIDTH                 ),
    .RESET_VAL                          (RESET_VAL                 ) 
) CSR_MEPC(
    .clock                              (clock                     ),
    .reset                              (reset                     ),
    .din                                (mepc_in                   ),
    .dout                               (mepc_out                  ),
    .wen                                (csr_wen[0] | ecall_flag   ) 
);

Reg #( //mcause寄存器
    .WIDTH                              (CSR_WIDTH                 ),
    .RESET_VAL                          (RESET_VAL                 ) 
) CSR_MCAUSE(
    .clock                              (clock                     ),
    .reset                              (reset                     ),
    .din                                (mcause_in                 ),
    .dout                               (mcause_out                ),
    .wen                                (csr_wen[1] | ecall_flag   ) 
);

Reg #( //mstatus寄存器
    .WIDTH                              (CSR_WIDTH                 ),
    .RESET_VAL                          (32'h1800                  ) 
) CSR_MSTATUS(
    .clock                              (clock                     ),
    .reset                              (reset                     ),
    .din                                (mstatus_in                ),
    .dout                               (mstatus_out               ),
    .wen                                (csr_wen[2]                ) 
);

Reg #( //metvec寄存器
    .WIDTH                              (CSR_WIDTH                 ),
    .RESET_VAL                          (RESET_VAL                 ) 
) CSR_MTVEC(
    .clock                              (clock                     ),
    .reset                              (reset                     ),
    .din                                (mtvec_in                  ),
    .dout                               (mtvec_out                 ),
    .wen                                (csr_wen[3]                ) 
);



endmodule                                                           //CSR
