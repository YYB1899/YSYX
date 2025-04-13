import "DPI-C" function int pmem_read(input int raddr);
import "DPI-C" function void pmem_write( input int waddr, input int wdata,  input bit [3:0] wmask);
module memory_interface(
	input clk,
	input rst,
	input [31:0] alu_result,
	input [2:0] is_load,
	input [2:0] is_store,
	input [31:0] wdata,
	output [31:0] rdata,
	output use_wdata
);

	wire [3:0] wmask;
	reg [31:0] mem_rdata;
	
    	wire valid = (is_load != 3'b111) || (is_store != 3'b111);
    	wire wen = (is_store != 3'b111) && (is_load == 3'b111);
    	wire [31:0] addr = alu_result;
	
	assign use_wdata = valid;
	
	assign wmask = 
        	(is_store == 3'b000) ? sb_mask(addr[1:0]) :  // SB
        	(is_store == 3'b001) ? sh_mask(addr[1:0]) :  // SH
        	(is_store == 3'b010) ? 4'b1111 :             // SW        
   	4'b0000;                                      // 非Store指令

	// 生成SB的字节掩码（支持非对齐）
	function [3:0] sb_mask(input [1:0] addr_lsb);
    		case (addr_lsb)
        		2'b00: return 4'b0001;
        		2'b01: return 4'b0010;
        		2'b10: return 4'b0100;
        		2'b11: return 4'b1000;
    		endcase
	endfunction

	// 生成SH的半字掩码（支持非对齐）
	function [3:0] sh_mask(input [1:0] addr_lsb);
    		case (addr_lsb)
        		2'b00: return 4'b0011;  // 低半字
        		2'b10: return 4'b1100;  // 高半字
        		default: return 4'b0000; // 非对齐触发异常
    		endcase
	endfunction 
	
	always @(*) begin
        	if (valid) begin
        		mem_rdata = pmem_read(addr);
        	if (wen) begin
                	pmem_write(addr, wdata, wmask);
            	end
           	end else begin
           		mem_rdata = 32'b0;
          	end
    	end
	
	wire [7:0]  loaded_byte;
    	wire [15:0] loaded_half;
    
    	// 根据地址低2位选择数据
    	assign loaded_byte = 
        	(addr[1:0] == 2'b00) ? mem_rdata[7:0] :
        	(addr[1:0] == 2'b01) ? mem_rdata[15:8] :
        	(addr[1:0] == 2'b10) ? mem_rdata[23:16] : 
        	mem_rdata[31:24];
    
    	assign loaded_half = 
        	addr[1] ? mem_rdata[31:16] : mem_rdata[15:0];
    
   	// 根据加载类型选择扩展方式
    	assign rdata = 
        	(is_load == 3'b000) ? {{24{loaded_byte[7]}}, loaded_byte} :  // LB（符号扩展）
        	(is_load == 3'b001) ? {{16{loaded_half[15]}}, loaded_half} : // LH（符号扩展）
        	(is_load == 3'b010) ? mem_rdata :                            // LW（直接使用）
        	(is_load == 3'b100) ? {24'b0, loaded_byte} :                 // LBU（零扩展）
        	(is_load == 3'b101) ? {16'b0, loaded_half} :                 // LHU（零扩展）
        32'b0;                                                      // 默认

endmodule
