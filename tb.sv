`default_nettype none
`timescale 1ns / 1ns

module tb #(
	parameter max_recvpkt = 3,
	parameter nPreamble = 8,
	parameter nIFG = 12
)();

import "DPI-C" context function int pipe_init();
import "DPI-C" context task pipe_release();
import "DPI-C" context task tap2gmii(output int ret);
export "DPI-C" task gmii_write;
export "DPI-C" task gmii_preamble;
export "DPI-C" task gmii_ifg;

// clock
logic clk_125m;
initial clk_125m = 0;
always #1 begin
	clk_125m = ~clk_125m;
end
default clocking clk @(posedge clk_125m);
endclocking

logic rst_n, fifo_dv;
logic [7:0] fifo_din;
wire gmii_en;
wire [7:0] gmii_dout;
hub hub0 (
		.sys_rst(~rst_n)
	,	.gmii_gtx_clk(clk_125m)
	,	.*
);


int ret, pkt_count;
initial begin
	$dumpfile("wave.vcd");
	$dumpvars(0, hub0);
	fifo_din = 8'h0;
	fifo_dv = 1'b0;
	rst_n = 1'b0;

	ret <= pipe_init();
	if (ret < 0) begin
		$display("pipe_init: open: ret < 0");
	end

	##5;
	rst_n <= 1;
	pkt_count <= max_recvpkt;
	while(1) begin
		tap2gmii(ret);
		if (ret == 1) begin
			if (!(--pkt_count)) begin
				break;
			end
		end
	end

	##10;
	pipe_release();
	$finish;
end


/*
 * gmii_write
 * @data
 */
task gmii_write(input byte data);
	@(posedge clk_125m) begin
		fifo_dv <= 1'b1;
		fifo_din <= data;
	end
endtask

/*
 * gmii_preamble
 */
task gmii_preamble;
	for (int i = 1; i <= nPreamble; i++) begin
		@(posedge clk_125m) begin
			fifo_dv <= 1'b1;
			if (i != nPreamble) begin
				fifo_din <= 8'h55;
			end else begin
				fifo_din <= 8'hd5;
			end
		end
	end
endtask

/*
 * gmii_ifg
 */
task gmii_ifg;
	for (int i = 0; i < nIFG; i++) begin
		@(posedge clk_125m) begin
			fifo_dv <= 1'b0;
			fifo_din <= 8'h0;
		end
	end
endtask

endmodule

`default_nettype wire

