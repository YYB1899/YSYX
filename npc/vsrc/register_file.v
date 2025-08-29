module register_file #(ADDR_WIDTH = 5, DATA_WIDTH = 32) (
  input clk,
  input [DATA_WIDTH-1:0] wdata,
  input [ADDR_WIDTH-1:0] rs1,
  input [ADDR_WIDTH-1:0] rs2,
  input [ADDR_WIDTH-1:0] rd,
  input wen,
  output [DATA_WIDTH-1:0] rs1_data,
  output [DATA_WIDTH-1:0] rs2_data
);
  reg [DATA_WIDTH-1:0] rf [2**ADDR_WIDTH-1:0];
  
  integer i;
  initial begin	
  	for (i = 0 ; i < 2**ADDR_WIDTH ; i = i + 1)begin
  		rf[i] = 0;
  	end
  end
  
  // 导出DPI-C函数
  import "DPI-C" context function void register_file_scope();
  export "DPI-C" function get_reg_value;
  
  function int get_reg_value(input int reg_num);
    if (reg_num >= 0 && reg_num < 2**ADDR_WIDTH)
      return rf[reg_num];
    else
      return 0;
  endfunction
  
  initial begin
    register_file_scope(); // 设置当前模块为DPI上下文
  end
  
  assign rs1_data = (rs1 == 0) ? 0 : rf[rs1];
  assign rs2_data = (rs2 == 0) ? 0 : rf[rs2];
  
  always @(posedge clk) begin
    if (wen && rd != 5'd0) begin  // 禁止写入x0寄存器
      rf[rd] <= wdata;
      // 调试信息：跟踪寄存器写入，特别关注x1寄存器
      if (rd == 5'd1) begin
        $display("REG DEBUG: Writing to x1 (ra): %h", wdata);
      end
        $display("REG DEBUG: x%d <= %h", rd, wdata);
    end else if (wen && rd == 5'd0) begin
      $display("REG DEBUG: Attempted write to x0 (ignored): %h", wdata);
    end
    
    // 调试信息：当读取x1寄存器时显示其值
    if (rs1 == 5'd1) begin
      $display("REG DEBUG: Reading x1 (ra): %h", rs1_data);
    end
  end
endmodule
