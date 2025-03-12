module alu(      
    input wire [31:0] r1,       
    input wire [31:0] r2,         
    input wire [2:0] sub,              
    output reg [31:0] sum,      
    output reg overflow
);      
      
    reg [32:0] temp_sum;      
    reg [31:0] r2_complement;    
    reg [31:0] s;
    
always @(*) begin    
    case(sub)   
        3'b000: begin    
            temp_sum = {1'b0, r1} + {1'b0, r2};    
            sum = temp_sum[31:0];      
            r2_complement = 32'b0; 
            overflow = (~sum[31]&r1[31]&r2[31]) | (sum[31]&(~r1[31])&(~r2[31]));
            s = 32'b0;
        end    
        3'b001: begin    
            r2_complement = ~r2 + 1'b1;    
            temp_sum = {1'b0, r1} + {1'b0, r2_complement};  
            sum = temp_sum[31:0];    
   	    overflow =  (~sum[31]&r1[31]&(~r2[31])) | (sum[31]&(~r1[31])&r2[31]);
   	    s = 32'b0;
        end    
        3'b010: begin    
            sum = ~r1;   
            temp_sum = 33'b0;
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
        end    
        
        3'b011: begin  
            sum = r1 & r2;  
            temp_sum = 33'b0;
            r2_complement = 32'b0; 
            overflow = 1'b0;
            s = 32'b0;
        end  
        3'b100: begin  
            sum = r1 | r2;   
            temp_sum = 33'b0;
            r2_complement = 32'b0; 
            overflow = 1'b0;
            s = 32'b0;
        end  
        3'b101: begin  
            sum = r1 ^ r2;  
            temp_sum = 33'b0; 
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
        end  
        3'b110: begin  
            r2_complement = ~r2 + 1'b1;    
            temp_sum = {1'b0, r1} + {1'b0, r2_complement};  
            s = temp_sum[31:0];    
   	    overflow =  (~s[31]&r1[31]&(~r2[31])) | (s[31]&(~r1[31])&r2[31]);
   	    if((overflow == 0 && s[31] == 1) || (overflow == 1 && s[31] == 0)) begin
   	    	sum = 32'b1;
   	    end else sum = 32'b0;
        end   
         3'b111: begin  
            sum = (r1 == r2) ? 32'b1 : 32'b0;   
            temp_sum = 33'b0;
            r2_complement = 32'b0;
            overflow = 1'b0;
            s = 32'b0;
        end  
    endcase    
end    
endmodule
