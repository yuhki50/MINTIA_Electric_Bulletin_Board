`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   20:34:59 06/11/2011
// Design Name:   top
// Module Name:   C:/Xilinx/Project/MINTIA_Electric_Bulletin_Board/top_bench.v
// Project Name:  MINTIA_Electric_Bulletin_Board
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: top
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module top_bench;

	// Inputs
	reg spi_mosi;
	reg spi_sclk;
	reg spi_ss;

	// Outputs
	wire sr_data;
	wire sr_clock;
	wire sr_latch;
	wire sr_enable;
	wire [13:0] fet_gate;

	// Instantiate the Unit Under Test (UUT)
	top uut (
		.spi_mosi(spi_mosi), 
		.spi_sclk(spi_sclk), 
		.spi_ss(spi_ss), 
		.sr_data(sr_data), 
		.sr_clock(sr_clock), 
		.sr_latch(sr_latch), 
		.sr_enable(sr_enable), 
		.fet_gate(fet_gate)
	);

	initial begin
		// Initialize Inputs
		spi_mosi = 0;
		spi_sclk = 0;
		spi_ss = 0;

		// Wait 100 ns for global reset to finish
//		#100;
        
		// Add stimulus here
		#10;
		spi_mosi <= 1;
		spi_ss <= 1;
		#9;
		spi_ss <= 0;
	end
	
	always begin
		spi_sclk <= 1;
		#2;
		spi_sclk <= 0;
		#2;
	end
endmodule

