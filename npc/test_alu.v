module test_alu;
    reg [31:0] r1, r2;
    reg [3:0] sub;
    reg alu_enable;
    wire [31:0] sum;
    wire overflow;
    
    alu alu_test (
        .r1(r1),
        .r2(r2),
        .sub(sub),
        .sum(sum),
        .overflow(overflow),
        .alu_enable(alu_enable)
    );
    
    initial begin
        alu_enable = 1;
        
        // 测试逻辑右移
        r1 = 32'h8;  // 8
        r2 = 32'h1;  // 移位量 1
        sub = 4'b1001; // SRLI
        
        #10;
        $display("SRLI Test: r1=%h, r2=%h, result=%h (expected: 4)", r1, r2, sum);
        
        r1 = 32'h4;  // 4
        #10;
        $display("SRLI Test: r1=%h, r2=%h, result=%h (expected: 2)", r1, r2, sum);
        
        r1 = 32'h2;  // 2
        #10;
        $display("SRLI Test: r1=%h, r2=%h, result=%h (expected: 1)", r1, r2, sum);
        
        r1 = 32'h1;  // 1
        #10;
        $display("SRLI Test: r1=%h, r2=%h, result=%h (expected: 0)", r1, r2, sum);
        
        $finish;
    end
endmodule 