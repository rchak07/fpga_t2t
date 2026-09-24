module msg_parser(
    input logic clk,
    input logic rst,
    input logic in_valid,
    input logic [7:0] in_byte,

    output logic out_valid,
    output logic side,
    output logic [31:0] price,
    output logic [15:0] qty
);

    logic [2:0] byte_cnt;

    always_ff @(posedge clk) begin
        if(rst) begin
            byte_cnt <= 3'd0;
            out_valid <= 1'b0;
            side <= 1'b0;
            price <= 32'd0;
            qty <= 16'd0;
        end else begin
            out_valid <= 1'b0; // Default to not valid unless we complete a message
            if (in_valid) begin
                case(byte_cnt)
                    3'd0: begin
                        if(in_byte == 8'h51) begin
                            byte_cnt <= 3'd1;
                        end
                    end
                    3'd1: begin
                        side = in_byte[0]; // Assuming side is determined by the LSB of the second byte
                        byte_cnt <= 3'd2;
                    end
                    3'd2: begin
                        price[31:24] = in_byte; // First byte of price
                        byte_cnt <= 3'd3;
                    end
                    3'd3: begin
                        price[23:16] = in_byte; // Second byte of price
                        byte_cnt <= 3'd4;
                    end
                    3'd4: begin
                        price[15:8] = in_byte; // Third byte of price
                        byte_cnt <= 3'd5;
                    end
                    3'd5: begin
                        price[7:0] = in_byte; // Fourth byte of price
                        byte_cnt <= 3'd6;
                    end
                    3'd6: begin
                        qty[15:8] = in_byte; // First byte of quantity
                        byte_cnt <= 3'd7;
                    end
                    3'd7: begin
                        qty[7:0] = in_byte; // Second byte of quantity
                        out_valid <= 1'b1; // Message complete, output valid
                        byte_cnt <= 3'd0; // Reset for next message
                    end
                    default: begin
                        byte_cnt <= 3'd0; // Reset on unexpected state
                    end
                endcase
            end
        end
    end
endmodule
