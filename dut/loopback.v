`default_nettype none

module hub (
		input wire       sys_rst
	,	input wire       gmii_gtx_clk
	,	input wire       fifo_dv
	,	input wire [7:0] fifo_din
	,	output reg       gmii_en
	,	output reg [7:0] gmii_dout
);

always @(posedge gmii_gtx_clk) begin
	if (sys_rst) begin
		gmii_en   <= 1'b0;
		gmii_dout <= 8'b0;
	end else begin
		gmii_en <= 1'b0;
		if (fifo_dv) begin
			gmii_en <= 1'b1;
		end
		gmii_dout <= fifo_din;
	end
end
endmodule

`default_nettype wire

