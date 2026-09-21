


module tb;

    `include "test_vectors.svh"

    logic clk, rst, start;
    logic [7:0] f0, f1, f2, f3;
    logic done;
    logic [7:0] predicted_class;



    tree_walker dut (
        .clk(clk),
        .rst(rst),
        .start(start),
        .f0(f0),
        .f1(f1),
        .f2(f2),
        .f3(f3),
        .done(done),
        .predicted_class(predicted_class)
    );



    initial clk = 0;
    always #5 clk = ~clk; 

    initial begin
        rst = 1;
        #10;
        rst = 0;

        for (int i = 0; i < NUM_VECTORS; i++) begin
            f0 = TEST_VECTORS[i][0];
            f1 = TEST_VECTORS[i][1];
            f2 = TEST_VECTORS[i][2];
            f3 = TEST_VECTORS[i][3];
            @(negedge clk);
            start = 1;
            @(negedge clk);
            start = 0;
            @(negedge clk);
            wait(done);
            if(predicted_class == TEST_VECTORS[i][4]) begin
                $display("Test %0d passed: predicted_class = %0d, expected_class = %0d", i, predicted_class, TEST_VECTORS[i][4]);
            end
            else begin
                $display("Test %0d failed: predicted_class = %0d, expected_class = %0d", i, predicted_class, TEST_VECTORS[i][4]);
            end
        end
        $finish;
    end
endmodule
