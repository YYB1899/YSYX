module alu(      
    input wire [31:0] r1,       
    input wire [31:0] r2,         
    input wire [3:0] sub,              
    output reg [31:0] sum,      
    output reg overflow,
    input wire alu_enable //enable=1正常执行ALU操作
);      
      
    reg [32:0] temp_sum;      
    reg [31:0] r2_complement;    
    reg [31:0] s;
    
always @(*) begin  
    if(!alu_enable) begin
    	    sum = 32'b0;
    	    overflow = 1'b0;
            temp_sum = 33'b0;
            r2_complement = 32'b0;
            s = 32'b0;
        end
    else begin  
    case(sub)   
        4'b0000: begin    // ADD
            temp_sum = {1'b0, r1} + {1'b0, r2};    
            sum = temp_sum[31:0];      
            r2_complement = 32'b0; 
            overflow = (~sum[31]&r1[31]&r2[31]) | (sum[31]&(~r1[31])&(~r2[31]));
            s = 32'b0;
            // 调试输出
            if (alu_enable) begin
                //$display("ALU DEBUG: ADD r1=0x%08x, r2=0x%08x, result=0x%08x", r1, r2, sum);
            end
        end    
        4'b0001: begin    // SUB
            r2_complement = ~r2 + 1'b1;    
            temp_sum = {1'b0, r1} + {1'b0, r2_complement};  
            sum = temp_sum[31:0];    
   	    overflow =  (~sum[31]&r1[31]&(~r2[31])) | (sum[31]&(~r1[31])&r2[31]);
   	    s = 32'b0;
            // 调试输出
            if (alu_enable) begin
                //$display("ALU DEBUG: SUB r1=0x%08x, r2=0x%08x, result=0x%08x", r1, r2, sum);
            end
        end    
        4'b0010: begin    
            sum = ~r1;   
            temp_sum = 33'b0;
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
        end    
        4'b0011: begin  
            sum = r1 & r2;  
            temp_sum = 33'b0;
            r2_complement = 32'b0; 
            overflow = 1'b0;
            s = 32'b0;
            if (alu_enable) begin
                //$display("ALU DEBUG: AND r1=0x%08x, r2=0x%08x, result=0x%08x", r1, r2, sum);
            end
        end  
        4'b0100: begin  
            sum = r1 | r2;   
            temp_sum = 33'b0;
            r2_complement = 32'b0; 
            overflow = 1'b0;
            s = 32'b0;
            if (alu_enable) begin
                //$display("ALU DEBUG: OR r1=0x%08x, r2=0x%08x, result=0x%08x", r1, r2, sum);
            end
        end  
        4'b0101: begin  
            sum = r1 ^ r2;  
            temp_sum = 33'b0; 
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
            if (alu_enable) begin
                //$display("ALU DEBUG: XOR r1=0x%08x, r2=0x%08x, result=0x%08x", r1, r2, sum);
            end
        end  
	4'b0110: begin  // 带符号数比较 (r1 >= r2 ? 0 : 1)
    		r2_complement = ~r2 + 1'b1;    // 计算r2的补码
    		temp_sum = {1'b0, r1} + {1'b0, r2_complement};  // r1 - r2
    		s = temp_sum[31:0];    // 取结果的低32位  
   		// 判断r1 >= r2
    		if (r1[31] == r2[31]) begin  // 符号相同
        		sum = (temp_sum[31] == 0) ? 32'b0 : 32'b1;  // 直接比较结果
    		end
    		else begin  // 符号不同
        		sum = (r1[31] == 0) ? 32'b0 : 32'b1;  // 正数肯定大于负数
    		end
    		overflow = (~s[31]&r1[31]&(~r2[31])) | (s[31]&(~r1[31])&r2[31]);
    		// 添加调试信息
    		if (alu_enable) begin
    		    //$display("ALU DEBUG: BGE r1=0x%08x, r2=0x%08x, r1[31]=%b, r2[31]=%b, temp_sum=0x%09x, result=0x%08x", 
    		             //r1, r2, r1[31], r2[31], temp_sum, sum);
    		end
	end  
	4'b0111: begin  // SLTIU - 无符号数比较 (r1 < r2 ? 1 : 0)
    	    temp_sum = {1'b0, r1} - {1'b0, r2};  // 33位减法保留借位
    	    sum = temp_sum[32] ? 32'b1 : 32'b0;  // 若发生借位（r1 < r2），sum=1
    	    r2_complement = 32'b0;
    	    overflow = 1'b0;
    	    s = 32'b0;
            // 调试输出 - 特别关注SLTIU操作
            if (alu_enable) begin
                //$display("ALU DEBUG: SLTIU r1=0x%08x, r2=0x%08x, temp_sum=0x%09x, borrow=%d, result=0x%08x", 
                         //r1, r2, temp_sum, temp_sum[32], sum);
                if (r2 == 32'h1) begin
                    //$display("ALU DEBUG: SEQZ operation - checking if r1==0, r1=0x%08x, result=%d", r1, sum);
                end
            end
	end  
	4'b1000: begin  // SLL/SLLI（逻辑左移）
            sum = r1 << r2[4:0];          // 实际位移结果
            temp_sum = 33'b0;      
            r2_complement = 32'b0;     
            overflow = 1'b0;   
            s = 32'b0;
            if (alu_enable) begin
                //$display("ALU DEBUG: SLL r1=0x%08x, shift=%d, result=0x%08x", r1, r2[4:0], sum);
            end                
        end
	4'b1001: begin  // SRL/SRLI（逻辑右移）
            sum = r1 >> r2[4:0];          // 高位补0
            temp_sum = 33'b0;
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
            if (alu_enable) begin
                //$display("ALU DEBUG: SRL r1=0x%08x, shift=%d, result=0x%08x", r1, r2[4:0], sum);
            end
        end
	4'b1010: begin  // SRA/SRAI（算术右移）
            sum = $signed(r1) >>> r2[4:0]; // 高位符号扩展
            temp_sum = 33'b0;
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
        end
        4'b1011: begin 
            sum = (r1 == r2) ? 32'b1 : 32'b0;  // 相等为1，不等为0
    	    temp_sum = 33'b0;
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
            // 调试输出
            if (alu_enable) begin
                //$display("ALU DEBUG: EQ r1=0x%08x, r2=0x%08x, result=0x%08x", r1, r2, sum);
            end
        end
        4'b1100: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
        4'b1101: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
        4'b1110: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
        4'b1111: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
    endcase   
    end
    end    
endmodule
