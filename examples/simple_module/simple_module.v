module simple_module (
  input clk,
  input a,
  input b,
  output c,
  output d
);

assign c = ~(a & b);

//assign d = a & b;
assign d = 1;

reg q;

initial begin
  $aig_bad(a,"hi");
end

always @ ( posedge clk ) begin

  if ( a ) begin
    if (d )
      q <= ~( a | b & c);
  end
end

endmodule
