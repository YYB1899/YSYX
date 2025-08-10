module memory_interface(
	input clk,
	input rst,
	input [31:0] alu_result,
	input [2:0] is_load,
	input [2:0] is_store,
	input [31:0] wdata,
	output reg [31:0] rdata,
	output use_wdata
);
	import "DPI-C" function int pmem_read(input int raddr);
	import "DPI-C" function void pmem_write( input int waddr, input int wdata,  input bit [3:0] wmask);
	wire [3:0] wmask;
	reg [31:0] mem_rdata;
	
	// 添加一个简单的内部内存来处理栈区域
	reg [31:0] stack_mem [0:1023]; // 4KB 栈内存
	wire is_stack_addr = (alu_result >= 32'h80008000) && (alu_result < 32'h80009000);
	wire [31:0] stack_offset = (alu_result - 32'h80008000) >> 2; // 字地址偏移
	wire [9:0] stack_index = stack_offset[9:0]; // 取低10位作为索引
	
	initial begin
		integer i;
		for (i = 0; i < 1024; i = i + 1) begin
			stack_mem[i] = 32'h00000000;
		end
	end
	
    	wire valid = (is_load != 3'b111) || (is_store != 3'b111);
    	wire ren = (is_load != 3'b111) && (is_store == 3'b111);
    	wire wen = (is_store != 3'b111) && (is_load == 3'b111);
    	wire [31:0] addr = alu_result;

	assign use_wdata = (is_load != 3'b111);
	
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
		
	// 读操作使用组合逻辑（单周期CPU需要）
	always @(*) begin
		mem_rdata = 32'b0;
		if (ren == 1'b1) begin // 有读请求时
			// 写入转发：如果同时有写操作到相同地址，直接返回写入数据
			if (wen == 1'b1 && addr == alu_result) begin
				mem_rdata = wdata;
				$display("WRITE FORWARDING: addr=%h, forwarded_data=%h", addr, wdata);
			end else if (is_stack_addr) begin
				// 使用内部栈内存
				mem_rdata = stack_mem[stack_index];
				$display("STACK READ: addr=%h, index=%d, data=%h", addr, stack_index, stack_mem[stack_index]);
			end else begin
				// 使用 DPI-C 函数
				mem_rdata = pmem_read(addr);
				if (addr == 32'h80000154) begin
					$display("VERILOG DEBUG: addr=0x%08x, pmem_read returned=0x%08x", addr, mem_rdata);
				end   
			end
			// 特别关注80008ff0地址的读取
			if (addr == 32'h80008ff0) begin
				$display("*** CRITICAL: Reading from 80008ff0, data=%h ***", mem_rdata);
			end
		end                                  
	end
	
	// 写操作使用时序逻辑，确保每个时钟周期只执行一次
	always @(posedge clk) begin
		if (!rst) begin
			if (wen == 1'b1) begin // 有写请求时
				if (is_stack_addr) begin
					// 写入内部栈内存 - 支持字节级写入
					case (is_store)
						3'b000: begin // SB - 字节存储
							case (addr[1:0])
								2'b00: stack_mem[stack_index][7:0]   <= wdata[7:0];
								2'b01: stack_mem[stack_index][15:8]  <= wdata[7:0];
								2'b10: stack_mem[stack_index][23:16] <= wdata[7:0];
								2'b11: stack_mem[stack_index][31:24] <= wdata[7:0];
							endcase
							$display("STACK BYTE WRITE: addr=%h, index=%d, byte_offset=%d, byte_data=%h", 
							         addr, stack_index, addr[1:0], wdata[7:0]);
						end
						3'b001: begin // SH - 半字存储
							case (addr[1])
								1'b0: stack_mem[stack_index][15:0]  <= wdata[15:0];
								1'b1: stack_mem[stack_index][31:16] <= wdata[15:0];
							endcase
							$display("STACK HALF WRITE: addr=%h, index=%d, half_offset=%d, half_data=%h", 
							         addr, stack_index, addr[1], wdata[15:0]);
						end
						3'b010: begin // SW - 字存储
							stack_mem[stack_index] <= wdata;
							$display("STACK WORD WRITE: addr=%h, index=%d, data=%h", addr, stack_index, wdata);
						end
						default: begin // 其他情况，不执行写入
							$display("STACK WRITE: Unsupported store type %b", is_store);
						end
					endcase
				end else begin
					// 使用 DPI-C 函数
					pmem_write(addr, wdata, wmask);
				end
				// 添加Store操作的调试信息
				$display("STORE DEBUG: addr=%h, wdata=%h, wmask=%b, is_store=%b", 
				         addr, wdata, wmask, is_store);
				// 特别关注80008ff0地址
				if (addr == 32'h80008ff0) begin
					$display("*** CRITICAL: Writing to 80008ff0, data=%h ***", wdata);
					// 注释掉立即验证，因为非阻塞赋值在同一时钟周期内不会生效
					// 在下一个时钟周期验证会更准确
				end
			end
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
    	always @(*) begin
    		rdata = (is_load == 3'b010) ? mem_rdata :                    // LW（直接使用）
        		(is_load == 3'b000) ? {{24{loaded_byte[7]}}, loaded_byte} :  // LB（符号扩展）
        		(is_load == 3'b001) ? {{16{loaded_half[15]}}, loaded_half} : // LH（符号扩展）
        		(is_load == 3'b100) ? {24'b0, loaded_byte} :                 // LBU（零扩展）
        		(is_load == 3'b101) ? {16'b0, loaded_half} :                 // LHU（零扩展）
        		32'b0;                                                      // 默认
    	end

	// 添加调试信息
	always @(posedge clk) begin
		if (!rst && use_wdata) begin
			$display("MEM DEBUG: use_wdata=1, is_load=%b, addr=%h, rdata=%h", is_load, addr, rdata);
		end
	end

endmodule
