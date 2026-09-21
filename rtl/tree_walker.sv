


module tree_walker (
    input logic clk, rst, start,
    input logic [7:0] f0, f1, f2, f3,
    output logic done,
    output logic [7:0] predicted_class
);
    
    `include "tree_data.svh"

    logic [7:0] current_node;


    always_ff @(posedge clk) begin
        if (rst || start) begin
            current_node <= 0;
            done <= 0;
        end
        else if (IS_LEAF[current_node]) begin
            done <= 1;
            predicted_class <= LEAF_CLASS[current_node];
        end
        else begin
            case (FEATURE_IDX[current_node])
                 0: begin
                      if (f0 < THRESHOLD[current_node])
                          current_node <= LEFT_CHILD[current_node];
                      else
                          current_node <= RIGHT_CHILD[current_node];
                 end
                 1: begin
                      if (f1 < THRESHOLD[current_node])
                          current_node <= LEFT_CHILD[current_node];
                      else
                          current_node <= RIGHT_CHILD[current_node];
                 end
                 2: begin
                      if (f2 < THRESHOLD[current_node])
                          current_node <= LEFT_CHILD[current_node];
                      else
                          current_node <= RIGHT_CHILD[current_node];
                 end
                 3: begin
                      if (f3 < THRESHOLD[current_node])
                            current_node <= LEFT_CHILD[current_node];
                      else
                          current_node <= RIGHT_CHILD[current_node];
                 end
                 default: begin
                        current_node <= 0;
                 end
            endcase
        end
    end
endmodule



    

    

