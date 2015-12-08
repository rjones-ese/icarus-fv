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

reg q;
assign d = q;

wire bad = ~(a&(d==b) | ~a&(d==c));

initial begin
  //AG(~bad)
  $aig_bad(bad,"Bad Condition");
end

always @ ( posedge clk ) begin
    q <= (a)? b : c;
end

endmodule
