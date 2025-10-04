module Reg_Stack(
    input                                  reset                      ,
    input                                  clock                      ,
    input              [  31: 0]           pc                         ,
    input                                  ecall_flag                 , //ecall标志

    input              [   3: 0]           rs1                        ,
    input              [   3: 0]           rs2                        ,
    input              [   3: 0]           rd                         , //目标寄存器
    input              [  31: 0]           rd_value                   , //写入目标寄存器的值


    input              [  31: 0]           csr_addr                   , //CSR寄存器的地址
    input                                  R_wen                      , //通用寄存器写使能
    input              [   3: 0]           csr_wen                    , //CSR寄存器写使能
    input              [  31: 0]           csrd                       , //CSR寄存器写入数据

    output             [  31: 0]           rs1_value                  ,
    output             [  31: 0]           rs2_value                  ,
    output             [  31: 0]           a0_value                   ,
    output             [  31: 0]           csrs                       , //CSR寄存器读出数据
    output             [  31: 0]           mepc_out                   ,
    output             [  31: 0]           mtvec_out                   
);

    wire               [  31: 0]        wdata                       ;

    wire               [  31: 0]        mcause_out                  ;
    wire               [  31: 0]        mstatus_out                 ;
    wire               [  31: 0]        mvendorid_out               ;
    wire               [  31: 0]        marchid_out                 ;


    assign                              wdata                       = (rd == 4'd0)? 32'd0:rd_value; //ao寄存器为0

    assign                       csrs                      = (csr_addr == 32'h341)? mepc_out        :
                                                             (csr_addr == 32'h342)? mcause_out      :
                                                             (csr_addr == 32'h300)? mstatus_out     :
                                                             (csr_addr == 32'h305)? mtvec_out       :
                                                             (csr_addr == 32'hf11)?mvendorid_out    :
                                                             (csr_addr == 32'hf12)?marchid_out:32'd0;



CSR #(32,0) CSR_inst( //CSR寄存器
    .clock                              (clock                     ),
    .reset                              (reset                     ),

    .pc                                 (pc                        ),
    .ecall_flag                         (ecall_flag                ),
    .csrd                               (csrd                      ),


    .csr_wen                            (csr_wen                   ),
    
    .mvendorid_out                      (mvendorid_out             ),
    .marchid_out                        (marchid_out               ),
    .mepc_out                           (mepc_out                  ),
    .mcause_out                         (mcause_out                ),
    .mstatus_out                        (mstatus_out               ),
    .mtvec_out                          (mtvec_out                 ) 


);

RegisterFile #(4, 32) Reg_inst( //通用寄存器
    .clock                              (clock                     ),
    .wdata                              (wdata                     ),
    .waddr                              (rd                        ),
    .wen                                (R_wen                     ),
    .reset                              (reset                     ),
    .rs1_addr                           (rs1                       ),
    .rs2_addr                           (rs2                       ),

    .rs1_value                          (rs1_value                 ),
    .rs2_value                          (rs2_value                 ),
    .a0_value                           (a0_value                  ) 
);


endmodule

