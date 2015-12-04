module simple_fsm_tb();

reg clk = 0;
simple_fsm dut(.clk(clk));

always
  #2 clk <= ~clk;
initial begin
  $iterate("hi");
  #100 $finish;
end

endmodule
