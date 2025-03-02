module ALU (      
    input wire [31:0] R1,       
    input wire [31:0] R2,         
    input wire [2:0] SUB,              
    output reg [31:0] SUM,      
    output reg OVERFLOW
);      
      
    reg [32:0] temp_sum;      
    reg [31:0] R2_complement;    
    reg [31:0] S;
    
always @(*) begin    
    case(SUB)   
        3'b000: begin    
            temp_sum = {1'b0, R1} + {1'b0, R2};    
            SUM = temp_sum[31:0];      
            R2_complement = 32'b0; 
            OVERFLOW = (~SUM[31]&R1[31]&R2[31]) | (SUM[31]&(~R1[31])&(~R2[31]));
            S = 32'b0;
        end    
        3'b001: begin    
            R2_complement = ~R2 + 1'b1;    
            temp_sum = {1'b0, R1} + {1'b0, R2_complement};  
            SUM = temp_sum[31:0];    
   	    OVERFLOW =  (~SUM[31]&R1[31]&(~R2[31])) | (SUM[31]&(~R1[31])&R2[31]);
   	    S = 32'b0;
        end    
        3'b010: begin    
            SUM = ~R1;   
            temp_sum = 33'b0;
            R2_complement = 32'b0;
            OVERFLOW = 1'b0;
            S = 32'b0;
        end    
        
        3'b011: begin  
            SUM = R1 & R2;  
            temp_sum = 33'b0;
            R2_complement = 32'b0; 
            OVERFLOW = 1'b0;
            S = 32'b0;
        end  
        3'b100: begin  
            SUM = R1 | R2;   
            temp_sum = 33'b0;
            R2_complement = 32'b0; 
            OVERFLOW = 1'b0;
            S = 32'b0;
        end  
        3'b101: begin  
            SUM = R1 ^ R2;  
            temp_sum = 33'b0; 
            R2_complement = 32'b0;
            OVERFLOW = 1'b0;
            S = 32'b0;
        end  
        3'b110: begin  
            R2_complement = ~R2 + 1'b1;    
            temp_sum = {1'b0, R1} + {1'b0, R2_complement};  
            S = temp_sum[31:0];    
   	    OVERFLOW =  (~S[31]&R1[31]&(~R2[31])) | (S[31]&(~R1[31])&R2[31]);
   	    if((OVERFLOW == 0 && S[31] == 1) || (OVERFLOW == 1 && S[31] == 0)) begin
   	    	SUM = 32'b1;
   	    end else SUM = 32'b0;
        end   
         3'b111: begin  
            SUM = (R1 == R2) ? 32'b1 : 32'b0;   
            temp_sum = 33'b0;
            R2_complement = 32'b0;
            OVERFLOW = 1'b0;
            S = 32'b0;
        end  
    endcase    
end    
endmodule
