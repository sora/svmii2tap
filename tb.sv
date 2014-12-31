`default_nettype none
`timescale 1ps / 1ps

module tb;

import "DPI-C" task pipe_init();
import "DPI-C" task pipe_release();

reg rst, phy_clk, fifo_en;
reg [7:0] fifo_din;
wire gmii_en;
wire [7:0] gmii_dout;
hub hub0 (
		.sys_rst(rst)
	,	.gmii_gtx_clk(phy_clk)
	,	.fifo_en(fifo_en)
	,	.fifo_din(fifo_din)
	,	.gmii_en(gmii_en)
	,	.gmii_dout(gmii_dout)
);

task nop(input int n);
	for (int i = 0; i < n; i++)
		@(posedge phy_clk);
endtask : nop


initial begin
	#1;
	pipe_init();
	pipe_release();
	$finish;
end

endmodule

`default_nettype wire

