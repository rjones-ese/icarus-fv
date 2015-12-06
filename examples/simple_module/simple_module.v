module simple_module (
  input clk,
  input a,
  input b,
  output c,
  output d
);

//assign c = ~(a & b);

//assign d = a & b;
assign d = a | q;

reg q;

always @ ( posedge clk ) begin
  q <= b;
  /*
  if (c)
    q <= ~a;
  else
    q <= 0;
  */

end

endmodule
