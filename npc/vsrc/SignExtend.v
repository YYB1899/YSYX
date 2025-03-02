modulmodule SignExtend (
    input  [11:0] imm,
    output reg [31:0] imm_extended 
);

    always @(*) begin
        imm_extended = {{20{imm[11]}}, imm};
    end

endmodule
