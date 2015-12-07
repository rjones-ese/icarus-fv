module simple_module (
  input clk,
  input a,
  input b,
  input r,
  output c,
  output d
);

assign c = ~(a & b);

//assign d = a & b;
assign d = 0;

reg q;

initial begin
  $aig_bad(d,"hi");
end

always @ ( posedge clk ) begin
    q <= (a)? b : ~a;
    /*
  if ( a ) begin
    if (d )
      q <= a;//1'b1;//~( a | b & c);
    else
      q <= a & b;
  end
    */
end

endmodule
