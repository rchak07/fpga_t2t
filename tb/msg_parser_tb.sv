module tb;
    
    logic clk, rst, in_valid;
    logic [7:0] in_byte;

    logic out_valid, side;
    logic [31:0] price;
    logic [15:0] qty;

    msg_parser dut (
        .clk(clk),
        .rst(rst),
        .in_valid(in_valid),
        .in_byte(in_byte),
        .out_valid(out_valid),
        .side(side),
        .price(price),
        .qty(qty)
    );

    task send_byte(input [7:0] b);
        @(posedge clk);
        in_valid <= 1;
        in_byte <= b;
        @(posedge clk);
        in_valid <= 0;
    endtask 

    initial clk = 0;
    always #5 clk = ~clk;

    localparam int NUM_BYTES = 16000;      // 2000 messages * 8 bytes
    logic [7:0] test_bytes [0:NUM_BYTES-1];

    initial $readmemh("data/test.hex", test_bytes);

    localparam int NUM_MSGS = 2000;
    int exp_side [0:NUM_MSGS-1];
    int exp_price [0:NUM_MSGS-1];
    int exp_qty [0:NUM_MSGS-1];


    initial begin
        int fd, idx, s, p, q, r;
        fd = $fopen("data/expected_updates.txt", "r");
        for(int i = 0; i < NUM_MSGS; i++) begin
            r = $fscanf(fd, "%d %d %d %d\n", idx, s, p, q);
            exp_side[i] = s;
            exp_price[i] = p;
            exp_qty[i] = q;
        end
        $fclose(fd);
    end
    
    int errors;
    initial begin
        rst = 1;
        in_valid = 0;
        in_byte = 0;
        repeat (3) @(posedge clk);
        rst = 0;

        // Test case 1: back-to-back messages
        send_byte(8'h51);
        
        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h27);

        send_byte(8'h10);

        send_byte(8'h01);

        send_byte(8'hF4);

        wait(out_valid);
        $display("out_valid: %b, side: %b, price: %d, qty: %d", out_valid, side, price, qty);




        send_byte(8'h51);

        send_byte(8'h01);

        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h27);

        send_byte(8'h15);

        send_byte(8'h00);

        send_byte(8'hFA);
        wait(out_valid);
        $display("out_valid: %b, side: %b, price: %d, qty: %d", out_valid, side, price, qty);




        send_byte(8'h51);

        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h27);

        send_byte(8'h0E);

        send_byte(8'h02);

        send_byte(8'hEE);
        wait(out_valid);
        $display("out_valid: %b, side: %b, price: %d, qty: %d", out_valid, side, price, qty);



        // Test case 2: gap mid-message
        send_byte(8'h51);

        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h00);

        @(posedge clk) in_valid <= 0;
        @(posedge clk);
        @(posedge clk);       
        send_byte(8'h27);

        send_byte(8'h10);

        send_byte(8'h01);

        send_byte(8'hF4);
        
        
        wait(out_valid);
        $display("out_valid: %b, side: %b, price: %d, qty: %d", out_valid, side, price, qty);

        // Test case 3: bad type byte before a real message

        send_byte(8'hFF);
        send_byte(8'h51);
        
        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h00);

        send_byte(8'h27);

        send_byte(8'h10);

        send_byte(8'h01);

        send_byte(8'hF4);

        wait(out_valid);
        $display("out_valid: %b, side: %b, price: %d, qty: %d", out_valid, side, price, qty);


        // Test case 4: real-data vs golden model

        
        errors = 0;
        for(int m = 0; m < NUM_MSGS; m++) begin
            for(int b = 0; b < 8; b++) begin
                send_byte(test_bytes[m*8+b]);
            end
            wait(out_valid);
            if(side != exp_side[m] || price != exp_price[m] || qty != exp_qty[m]) begin
                $display("MISMATCH at msg %0d: got side=%b price=%d qty=%d, expected side=%b price=%d qty=%d", m, side, price, qty, exp_side[m], exp_price[m], exp_qty[m]);
                errors++;
            end
        end
        $display("Test 4 done: %0d/%0d messages detected", NUM_MSGS-errors, NUM_MSGS);



        


        $finish;


        

    end
endmodule



        


        