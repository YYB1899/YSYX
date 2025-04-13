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
        4'b0000: begin    
            temp_sum = {1'b0, r1} + {1'b0, r2};    
            sum = temp_sum[31:0];      
            r2_complement = 32'b0; 
            overflow = (~sum[31]&r1[31]&r2[31]) | (sum[31]&(~r1[31])&(~r2[31]));
            s = 32'b0;
        end    
        4'b0001: begin    
            r2_complement = ~r2 + 1'b1;    
            temp_sum = {1'b0, r1} + {1'b0, r2_complement};  
            sum = temp_sum[31:0];    
   	    overflow =  (~sum[31]&r1[31]&(~r2[31])) | (sum[31]&(~r1[31])&r2[31]);
   	    s = 32'b0;
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
        end  
        4'b0100: begin  
            sum = r1 | r2;   
            temp_sum = 33'b0;
            r2_complement = 32'b0; 
            overflow = 1'b0;
            s = 32'b0;
        end  
        4'b0101: begin  
            sum = r1 ^ r2;  
            temp_sum = 33'b0; 
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
        end  
        4'b0110: begin  //带符号数
            r2_complement = ~r2 + 1'b1;    
            temp_sum = {1'b0, r1} + {1'b0, r2_complement};  
            s = temp_sum[31:0];    
   	    overflow =  (~s[31]&r1[31]&(~r2[31])) | (s[31]&(~r1[31])&r2[31]);
   	    sum = (r1[31] ^ r2[31]) ? (r1[31] ? 32'b1 : 32'b0) :  (temp_sum[31] ? 32'b1 : 32'b0); 
        end   
	4'b0111: begin  // 无符号数
    	    temp_sum = {1'b0, r1} - {1'b0, r2};  // 33位减法保留借位
    	    sum = temp_sum[32] ? 32'b1 : 32'b0;  // 若发生借位（r1 < r2），sum=1
    	    r2_complement = 32'b0;
    	    overflow = 1'b0;
    	    s = 32'b0;
	end  
	4'b1000: begin  // SLL/SLLI（逻辑左移）
            sum = r1 << r2[4:0];          // 实际位移结果
            temp_sum = 33'b0;      
            r2_complement = 32'b0;     
            overflow = 1'b0;   
            s = 32'b0;                
        end
	4'b1001: begin  // SRL/SRLI（逻辑右移）
            sum = r1 >> r2[4:0];          // 高位补0
            temp_sum = 33'b0;
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
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
        end
        4'b1100: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
        4'b1101: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
        4'b1110: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
        4'b1111: begin sum = 32'b0;temp_sum = 33'b0;r2_complement = 32'b0;overflow = 1'b0;s = 32'b0;end
    endcase   
    end
    end    
endmodule
