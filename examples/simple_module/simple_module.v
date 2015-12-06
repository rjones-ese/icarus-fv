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

wire bad_a = (a == q);

initial begin
  $aig_constraint(a,"hi");
end

always @ ( posedge clk ) begin

  if ( a ) begin
    if (d )
      q <= ~( a | b & c);
  end
  /*
  if (b)
    q <= ~a;
  else
    q <= 0;
  */


end

endmodule
