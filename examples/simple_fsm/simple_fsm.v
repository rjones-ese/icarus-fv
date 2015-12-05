module simple_fsm(
   input clk,
   input x, y,
   output a, b
);

other oth(.clk(clk), .x(x));

parameter S0 = 2'b00;
parameter S1 = 2'b01;
parameter S2 = 2'b10;
parameter S3 = 2'b11;

//assign {a,b} = state;
assign a = state[0];
assign b = state[1];

reg [1:0] state = S0;

always @ (posedge clk) begin
  case(state)
    S0: state <= (x)? S1 : S0;
    S1: state <= (x)? S0 : (y)? S3 : S1;
    S2: state <= (~(x | y))? S3 : S0;
    S3: state <= S0;
  endcase
end

endmodule

module other(input clk,input x);
reg b = 0;
always @ ( posedge clk )
  b <= x;
endmodule
