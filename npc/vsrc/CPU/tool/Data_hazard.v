/* Deal with the Data hazard */

module Data_hazard(
    input              [   4: 0] IDU_rs1                    ,
    input              [   4: 0] IDU_rs2                    ,

    input              [   4: 0] EXU_rd                     ,
    input              [   4: 0] MEM_rd                     ,

    input                        IDU_valid                  ,
    input                        EXU_valid                  ,
    input                        MEM_valid                  ,

    input                        MEM_mem_ren                , //MEM是否加载指令
    input                        EXU_R_Wen                  , //EXU是否写寄存器
    input                        MEM_R_Wen                  , //MEM是否加载寄存器

    output             [   1: 0] IDU_rs1_choice             , //输出RS1的数据
    output             [   1: 0] IDU_rs2_choice               //输出RS2的数据
);

assign IDU_rs1_choice = (EXU_R_Wen && (EXU_rd == IDU_rs1 && EXU_rd != 0 && IDU_valid && EXU_valid))? //EXU前递（ALU结果直接用）
                        2'b01:(MEM_R_Wen && (MEM_rd == IDU_rs1 && MEM_valid && IDU_valid) && (MEM_rd!=0))? //MEM前递（加载的数据直接用于ALU或LOAD）
                        (MEM_mem_ren? 2'b011:2'b10):2'b000; //无冒险

assign IDU_rs2_choice = (EXU_R_Wen && (EXU_rd == IDU_rs2 && EXU_rd != 0 && IDU_valid && EXU_valid))? 
                        2'b01:(MEM_R_Wen && (MEM_rd == IDU_rs2 && MEM_valid && IDU_valid) && (MEM_rd!=0))? 
                        (MEM_mem_ren? 2'b011:2'b10):2'b000;

endmodule                                                           //Aribter



