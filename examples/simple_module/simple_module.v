module simple_module (
  input clk,
  input a,
  input b,
  output c,
  output d
);

assign c = ~(a & b);
//assign d = a & b;
assign d = ~a;

/*
reg q;

assign b = ~ ( a & c ) | q;

always @ ( posedge clk ) begin
  if (a & c)
    q <= ~a;
  else
    q <= 0;

end
*/

endmodule
