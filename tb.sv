`default_nettype none
`timescale 1ps / 1ps

module tb #(
	parameter max_recvpkt = 3
)();

import "DPI-C" context function int pipe_init();
import "DPI-C" context task pipe_release();
import "DPI-C" context task tap2gmii(output int r);
export "DPI-C" task gmii_write;
export "DPI-C" task nop;

reg rst_n, phy_clk, fifo_dv;
reg [7:0] fifo_din;
wire gmii_en;
wire [7:0] gmii_dout;
hub hub0 (
		.sys_rst(~rst_n)
	,	.gmii_gtx_clk(phy_clk)
	,	.fifo_dv(fifo_dv)
	,	.fifo_din(fifo_din)
	,	.gmii_en(gmii_en)
	,	.gmii_dout(gmii_dout)
);

task nop();
	repeat(10) begin
		#1;
		fifo_dv <= 1'b0;
	end
endtask :nop

// clock
initial phy_clk = 0;
always #1
	phy_clk = ~phy_clk;

int ret, r;
int pkt_count;
initial begin
	$dumpfile("hoge.vcd");
	$dumpvars(0, tb);

	rst_n <= 0;

	ret <= pipe_init();
	if (ret < 0) begin
		$display("pipe_init: open: ret < 0");
	end
	#10;

	rst_n <= 1;
	pkt_count <= max_recvpkt;
	while(1) begin
		tap2gmii(r);
		if (r == 1) begin
			if (!(--pkt_count)) begin
				break;
			end
		end
	end

	#100;
	pipe_release();
	$finish;
end

task gmii_write(input byte data);
	#1;
	fifo_dv <= 1'b1;
	fifo_din <= data;
endtask

endmodule

`default_nettype wire

