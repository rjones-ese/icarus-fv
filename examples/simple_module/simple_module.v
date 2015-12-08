/*
*   Simple Module
*   A trivial example.
*/
module simple_module (
  input clk,
  input a,
  input b,
  input c,
  output d
);

assign d = q;

wire bad = ~(q==b);

wire my_con = (a);


reg q;

initial begin
  //AG(bad)
  $aig_bad(bad,"hi");
  $aig_constraint(my_con,"oh");
end

always @ ( posedge clk ) begin
    q <= (a)? b : c;
end

endmodule
