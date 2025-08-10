module top (
    input  wire        clk,          // 时钟信号
    input  wire        rst,          // 复位信号
    output wire [31:0] pc,           // 程序计数器
    output wire [31:0] instruction,  // 当前指令
    output wire        overflow     // ALU 溢出信号
);

    // 内部信号
    wire [4:0]  rs1, rs2, rd;       // 寄存器地址
    wire [31:0] rs1_data, rs2_data, rd_data; // 寄存器数据
    wire        reg_write;          // 寄存器写使能
    wire        alu_src;            // ALU 操作数选择
    wire [3:0]  alu_ctrl;           // ALU 控制信号
    wire [31:0] alu_result;         // ALU 计算结果
    wire [31:0] imm;                // 符号扩展后的立即数
    wire        wb_src;             // 写回数据选择 (0: ALU结果, 1: 立即数)
    wire        alu_enable;         // alu使能信号
    wire        alu_r1;             // AUIPC用PC，其他用rs1
    wire        is_jal;             // JAL 指令标志
    wire        is_jalr;            // JALR 指令标志
    wire [31:0] pc_jal;             // 输出JAL的PC+4值
    wire [2:0]  b_type;             // B-type 指令类型标志 
    wire        is_b;               // B-type 指令标志 
    wire [2:0]  is_load;            // Load 指令
    wire [2:0]  is_store;           // Store 指令
    wire        use_wdata;
    

    // 实例化 PC 模块    
    pc pc_inst (
        .clk         (clk),
        .rst         (rst),
        .pc_out      (pc),
        .is_jalr     (is_jalr),
        .is_jal      (is_jal),
        .imm         (imm),
        .rs1_data    (rs1_data),
        .pc_jal      (pc_jal),
        .b_type      (b_type),
        .is_b        (is_b),
        .sum         (alu_result)
    );

    // 使用 pmem_read 获取指令
imem imem_inst (
    .pc         (pc),          // 连接程序计数器
    .instruction(instruction) // 输出指令
);  
    // ebreak结束仿真
    ebreak_detector ebreak_detector_inst (
        .clk         (clk),
        .rst         (rst),
        .pc          (pc),
        .instruction (instruction)
    ); 
    
    // 实例化 Control Unit 模块
    control_unit control_unit_inst (
        .instruction (instruction),
        .imm         (imm),
        .rs1         (rs1),
        .rs2         (rs2),
        .rd          (rd),
        .reg_write   (reg_write),
        .alu_src     (alu_src),
        .alu_ctrl    (alu_ctrl),
        .wb_src      (wb_src),
        .alu_enable  (alu_enable),
        .alu_r1      (alu_r1),
        .is_jalr     (is_jalr),
        .is_jal      (is_jal),
        .b_type      (b_type),
        .is_b        (is_b),
        .is_load     (is_load),
        .is_store    (is_store)
    );

    // 实例化 Register File 模块
    register_file register_file_inst (
        .clk        (clk),
        .rs1        (rs1),
        .rs2        (rs2),
        .rd         (rd), 
        .wen        (reg_write),
        .wdata      ((use_wdata) ? rd_data : ((is_jal || is_jalr) ? pc_jal : (wb_src ? imm : alu_result))), 
        .rs1_data   (rs1_data),
        .rs2_data   (rs2_data)
    );
    
    // 添加调试信息
    always @(posedge clk) begin
        if (!rst && reg_write) begin
            $display("WB DEBUG: rd=%d, use_wdata=%b, is_jal=%b, is_jalr=%b, wb_src=%b", 
                     rd, use_wdata, is_jal, is_jalr, wb_src);
            $display("WB DEBUG: rd_data=%h, pc_jal=%h, imm=%h, alu_result=%h", 
                     rd_data, pc_jal, imm, alu_result);
            $display("WB DEBUG: final_wdata=%h", 
                     (use_wdata) ? rd_data : ((is_jal || is_jalr) ? pc_jal : (wb_src ? imm : alu_result)));
        end
        
        // 专门调试关键的算术指令
        if (!rst && alu_enable) begin
            // 检测ADD指令 (opcode=0110011, funct3=000, funct7=0000000)
            if (instruction[6:0] == 7'b0110011 && instruction[14:12] == 3'b000 && instruction[31:25] == 7'b0000000) begin
                $display("CRITICAL DEBUG: ADD instruction at PC=%h", pc);
                $display("  rs1=%d(0x%08x) + rs2=%d(0x%08x) = 0x%08x", 
                         rs1, rs1_data, rs2, rs2_data, alu_result);
            end
            
            // 检测SUB指令 (opcode=0110011, funct3=000, funct7=0100000)
            if (instruction[6:0] == 7'b0110011 && instruction[14:12] == 3'b000 && instruction[31:25] == 7'b0100000) begin
                $display("CRITICAL DEBUG: SUB instruction at PC=%h", pc);
                $display("  rs1=%d(0x%08x) - rs2=%d(0x%08x) = 0x%08x", 
                         rs1, rs1_data, rs2, rs2_data, alu_result);
            end
            
            // 检测SLTIU指令 (opcode=0010011, funct3=011) - 这就是SEQZ
            if (instruction[6:0] == 7'b0010011 && instruction[14:12] == 3'b011) begin
                $display("CRITICAL DEBUG: SLTIU/SEQZ instruction at PC=%h", pc);
                $display("  rs1=%d(0x%08x) < imm=%d ? result=0x%08x", 
                         rs1, rs1_data, imm, alu_result);
                if (imm == 32'h1) begin
                    $display("  This is SEQZ: checking if rs1==0, result should be %d", 
                             (rs1_data == 0) ? 1 : 0);
                end
            end
        end
        
        // 添加Load/Store指令的详细调试信息
        if (!rst && (is_load != 3'b111 || is_store != 3'b111)) begin
            $display("INST DEBUG: PC=%h, instruction=%h, is_load=%b, is_store=%b", 
                     pc, instruction, is_load, is_store);
            $display("INST DEBUG: rs1=%d, rs2=%d, rd=%d, imm=%h", 
                     rs1, rs2, rd, imm);
        end
    end

    // 实例化 ALU 模块
    alu alu_inst (
        .r1         (alu_r1 ? pc : rs1_data),
        .r2         (alu_src ? imm : rs2_data), // 选择立即数或 rs2_data
        .sub        (alu_ctrl), 
        .sum        (alu_result),
        .overflow   (overflow),
        .alu_enable (alu_enable)
    );
    
    trap trap(
        .clk(clk),
        .rst(rst),
        .pc(pc),
        .instruction(instruction),
        .overflow(overflow)
    );

    memory_interface memory_interface(
        .clk         (clk),
        .rst         (rst),
        .alu_result  (alu_result),
        .is_load     (is_load),
        .is_store    (is_store),
        .wdata       (rs2_data),
        .rdata       (rd_data),
        .use_wdata   (use_wdata)
    );
endmodule
