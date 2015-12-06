module simple_module (
  input clk,
  input a,
  input c,
  output b
);

reg q;

assign b = ~ ( a & c ) | q;

always @ ( posedge clk ) begin
  if (a & c)
    q <= ~a;
  else
    q <= 0;

end

endmodule
