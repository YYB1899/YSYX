module top (    
    input wire [31:0] A,     
    input wire [31:0] B,        
    output wire [31:0] SUM,   
);    
  assign SUM = A + B;
endmodule
