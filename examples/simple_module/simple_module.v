module simple_module (
  input clk,
  input a,
  input b,
  output c,
  output d
);

assign c = ~(a & b);

//assign d = a & b;
assign d = a | q;

reg q;

wire bad_a = 1;

initial begin
  $aig_bad(d,"hi");
end

always @ ( posedge clk ) begin

  q <= ~( a | b & c);
  /*
  if (b)
    q <= ~a;
  else
    q <= 0;
  */


end

endmodule
