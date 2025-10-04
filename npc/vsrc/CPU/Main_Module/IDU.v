`include "../define/para.v"

    localparam                          i1_NR_KEY                  = 10    ;
    localparam                          i1_KEY_LEN                 = 7     ;
    localparam                          i1_DATA_LEN                = 32    ;
    

module IDU(
    input                               clock                      ,
    input                               reset                      ,

    input              [  31: 0]        inst                       , //指令
    input              [  31: 0]        snpc                       , //顺序下一个PC
    input              [  31: 0]        pc                         ,

    input              [  31: 0]        rd_value                   , //通用寄存器写入数据
    input              [  31: 0]        csrd                       , //CSR寄存器写入数据
    input              [   4: 0]        rd                         ,
    input                               R_wen                      ,
    input              [   3: 0]        csr_wen                    ,

    input              [  31: 0]        EXU_rs1_in                 ,
    input              [  31: 0]        EXU_rs2_in                 ,
    //解决数据冒险
    output             [   4: 0]        rd_next                    ,
    output             [   2: 0]        funct3                     , //功能码
    output                              mret_flag                  , //mret标志
    output                              ecall_flag                 , //ecall标志
    output                              fence_i_flag               , //fence.i标志

    output             [  31: 0]        branch_pc                  , //分支地址
    output             [  31: 0]        rs1_value                  ,
    output             [  31: 0]        rs2_value                  ,
    
    output             [  31: 0]        add1_value                 ,
    output             [  31: 0]        add2_value                 ,
    
    output             [   3: 0]        csr_wen_next               ,
    output                              R_wen_next                 ,
    output             [  31: 0]        rd_value_next              , 
    //流水线控制信号
    output                              mem_wen                    , //写使能
    output                              mem_ren                    , //读使能
    output                              inv_flag                   , //反转标志
    output                              branch_flag                , //分支标志
    output                              jump_flag                  , //跳转标志

    output             [   3: 0]        alu_opcode                 , //ALU操作码

    output             [   4: 0]        rs1                        ,
    output             [   4: 0]        rs2                        ,
    output             [  31: 0]        a0_value                   ,
    output             [  31: 0]        mepc_out                   ,
    output             [  31: 0]        mtvec_out                  ,

`ifdef Performance_Count
    output             [  31: 0]        pc_next                    ,
    output reg         [  31: 0]        InstR_count                ,
    output reg         [  31: 0]        InstI_count                ,
    output reg         [  31: 0]        InstS_count                ,
    output reg         [  31: 0]        InstB_count                ,
    output reg         [  31: 0]        InstU_count                ,
    output reg         [  31: 0]        InstJ_count                ,
    output reg         [  31: 0]        InstM_count                ,
    output             [  31: 0]        inst_next                  ,

`endif
    input                               valid_last                 ,
    output                              ready_last                 ,
    // IFU <-> IDU
    input                               ready_next                 ,
    output                              valid_next  
    // IDU <-> EXU                
);
`ifdef Performance_Count
    always @(posedge clock) begin
        if(reset)begin
            InstR_count <= 0;
            InstI_count <= 0;
            InstS_count <= 0;
            InstB_count <= 0;
            InstU_count <= 0;
            InstJ_count <= 0;
            InstM_count <= 0;
        end
        else if (valid_next & ready_next) begin //握手成功计数
            InstR_count <= (opcode == `R_opcode)? InstR_count+1:InstR_count;
            InstI_count <= (opcode == `I0_opcode || opcode == `I1_opcode || opcode == `I2_opcode)? InstI_count+1 : InstI_count;
            InstS_count <= (opcode == `S_opcode)? InstS_count+1 : InstS_count;
            InstB_count <= (opcode == `B_opcode)? InstB_count+1 : InstB_count;
            InstU_count <= (opcode == `U0_opcode || opcode == `U1_opcode)? InstU_count+1 : InstU_count;
            InstJ_count <= (opcode == `J_opcode)? InstJ_count + 1 : InstJ_count;
            InstM_count <= (opcode == `M_opcode)? InstM_count + 1 : InstM_count;
        end
    end
    assign                              inst_next                   = inst;
    assign                              pc_next                     = pc;

`endif

    wire               [  31: 0]        csr_addr                    ;
    wire               [   6: 0]        oprand                      ; //操作码高7位（funct7）
    wire               [   6: 0]        opcode                      ; //操作码低7位

    wire               [  31: 0]        imm_I                       ;
    wire               [  31: 0]        imm_U                       ;
    wire               [  31: 0]        imm_R                       ;
    wire               [  31: 0]        imm_S                       ;
    wire               [  31: 0]        imm_B                       ;
    wire               [  31: 0]        imm_J                       ;
    wire               [  31: 0]        csrs                        ; //CSR寄存器读取值
    wire               [  31: 0]        imm                         ; //最终立即数

    assign                              ready_last                  = ready_next; //IDU的ready信号直接等于EXU的ready信号
    assign                              valid_next                  = valid_last; //IDU的valid信号直接等于IFU的valid信号
    //透传


    assign                              oprand                      = inst[31:25];
    assign                              opcode                      = inst[6:0];
    assign                              rs1                         = inst[19:15];
    assign                              rs2                         = inst[24:20];
    assign                              funct3                      = inst[14:12];//功能码
    assign                              rd_next                     = inst[11:7]; //目标寄存器

    assign                              ecall_flag                  = (inst == 32'b00000000000000000000000001110011);//ecall
    assign                              mret_flag                   = (inst == 32'b00110000001000000000000001110011);//mret
    assign                              fence_i_flag                = (inst == 32'b00000000000000000001000000001111);//fence.i

    assign                              csr_wen_next[0]             = (opcode == `M_opcode && imm == 32'h341);//mepc
    assign                              csr_wen_next[1]             = (opcode == `M_opcode && imm == 32'h342);//mecause
    assign                              csr_wen_next[2]             = (opcode == `M_opcode && imm == 32'h300);//mstatus
    assign                              csr_wen_next[3]             = (opcode == `M_opcode && imm == 32'h305);//mtvec
    //检测写操作
    assign                              R_wen_next                  = (opcode == `S_opcode || opcode == `B_opcode || opcode == 0)? 1'b0:1'b1; //寄存器写使能
    assign                              mem_wen                     = (opcode == `S_opcode); //内存写使能
    assign                              mem_ren                     = (opcode == `I0_opcode);//内存读使能

    assign                              jump_flag                   = (opcode == `I2_opcode || opcode == `J_opcode)? 1'b1:1'b0; //跳转标志

    assign                              inv_flag                    = (opcode == `B_opcode && (funct3 == 3'b101 || funct3 == 3'b111 || funct3 == 3'b000 ))? 1'b1:1'b0; //反转标志
    assign                              branch_flag                 = (opcode == `B_opcode)? 1'b1:1'b0; //分支标志
 
    assign                              csr_addr                    = imm; //M指令，CSR寄存器地址为立即数

    assign                              rd_value_next               = jump_flag? snpc: 
                                                                    (|csr_wen_next)? csrs:
                                                                    0; //写回数据选择
    assign                              branch_pc                   = pc + imm; //分支PC


    assign add1_value = (opcode == `U0_opcode)? 0 :
                        (opcode == `J_opcode || opcode == `U1_opcode )? pc :
                        EXU_rs1_in;

    assign add2_value = (opcode == `R_opcode || opcode == `B_opcode)?  EXU_rs2_in :
                        (opcode == `M_opcode && funct3 == 3'b010)? rd_value_next :
                        (opcode == `M_opcode && funct3 == 3'b001)? 0 : imm;
 

    assign alu_opcode = (opcode == `S_opcode ||  opcode == `I0_opcode 
                        || opcode == `U0_opcode || opcode == `U1_opcode
                        || opcode == `J_opcode || opcode == `I2_opcode
                        || (opcode ==`I1_opcode  &&  funct3 == 3'b000)  || (opcode == `R_opcode         &&
                        funct3 == 3'b000 && oprand[5] == 1'b0) || (opcode == `B_opcode                             &&
                        funct3[2:1] == 2'b01                 ))                                                                     ?
                        `alu_add :(opcode == `I1_opcode && funct3 == 3'b010)                            ||
                        (opcode == `R_opcode && funct3 == 3'b010)                                                     ||
                        (opcode == `B_opcode && (funct3 == 3'b101 || funct3 == 3'b100))                               ?
                        `alu_signed_comparator:
                        (opcode == `B_opcode && (funct3 == 3'b110 || funct3 == 3'b111))                               ||
                        (opcode == `I1_opcode && (funct3 == 3'b011))                                                  ||
                        (opcode == `R_opcode && (funct3 ==  3'b011))                                                  ?
                        `alu_unsigned_comparator:
                        (opcode == `I1_opcode && funct3 == 3'b100 )                                                   ||
                        (opcode == `R_opcode && funct3 == 3'b100 )                                                    ?
                        `alu_xor :(opcode == `I1_opcode && funct3 == 3'b110 )                           ||
                        (opcode == `R_opcode && funct3 == 3'b110 )                                                    ||
                        (opcode == `M_opcode && funct3 == 3'b010 )                                                    ?
                        `alu_or  : (opcode == `I1_opcode && funct3 == 3'b111 )                          ||
                        (opcode == `R_opcode && funct3 == 3'b111 )                                                    ?
                        `alu_and :(opcode == `I1_opcode && funct3 == 3'b001  )                          ||
                        (opcode == `R_opcode && funct3 == 3'b001 )                                                    ?
                        `alu_sll :(opcode == `I1_opcode && funct3 == 3'b101 && oprand[5] == 1'b0)    ||
                        (opcode == `R_opcode && funct3 == 3'b101 && oprand[5] == 1'b0)                             ?
                        `alu_srl :(opcode == `I1_opcode && funct3 == 3'b101 && oprand[5] == 1'b1)    ||
                        (opcode == `R_opcode && funct3 == 3'b101 && oprand[5] == 1'b1)                             ?
                        `alu_sra : (opcode == `R_opcode && funct3 == 3'b000 && oprand[5] == 1'b1)    ?
                        `alu_sub : (opcode == `B_opcode && funct3[2:1] == 2'b00)                        ?
                        `alu_equal:`alu_add;


    assign                              imm_I                       = {{20{inst[31]}},inst[31:20]};
    assign                              imm_U                       = {inst[31:12],12'd0};
    assign                              imm_R                       = {25'd0,inst[31:25]};
    assign                              imm_S                       = {{20{inst[31]}},inst[31:25],inst[11:7]};
    assign                              imm_B                       = {imm_S[31:11],imm_S[0],imm_S[10:1]}<<1;
    /* verilator lint_off WIDTHEXPAND */
    assign                              imm_J                       = {{11{inst[31]}},inst[31],inst[19:12],inst[20],inst[30:21]}<<1;
    /* verilator lint_on WIDTHEXPAND */

    assign imm = (opcode == `I0_opcode || opcode == `I1_opcode || opcode == `I2_opcode || opcode == `M_opcode)? imm_I:
                 (opcode == `U0_opcode || opcode == `U1_opcode)? imm_U:
                 (opcode == `J_opcode)? imm_J:
                 (opcode == `B_opcode)? imm_B:
                 (opcode == `S_opcode)? imm_S: 
                 (opcode == `S_opcode)? imm_R :
                  0;

Reg_Stack Reg_Stack_inst0( //实例化寄存器堆
    .reset                              (reset                     ),
    .clock                              (clock                     ),
    .pc                                 (pc                        ),
    .ecall_flag                         (ecall_flag                ),

    .rs1                                (rs1[3:0]                  ),
    .rs2                                (rs2[3:0]                  ),
    .rd                                 (rd[3:0]                   ),
    .rd_value                           (rd_value                  ),

    .csr_addr                           (csr_addr                  ),
    .R_wen                              (R_wen                     ),
    .csr_wen                            (csr_wen                   ),
    .csrd                               (csrd                      ),

    .rs1_value                          (rs1_value                 ),
    .rs2_value                          (rs2_value                 ),
    .a0_value                           (a0_value                  ),
    .csrs                               (csrs                      ),
    .mepc_out                           (mepc_out                  ),
    .mtvec_out                          (mtvec_out                 ) 
);





endmodule

