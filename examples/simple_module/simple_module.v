module simple_module (
  input clk,
  input a,
  input b,
  input whoa,
  output c,
  output d
);

assign c = ~(a & b & whoa);

//assign d = a & b;
assign d = (c | a);

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
