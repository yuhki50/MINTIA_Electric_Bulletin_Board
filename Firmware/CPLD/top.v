`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    14:37:00 06/03/2011 
// Design Name: 
// Module Name:    top 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module top(
	/** pin assign **/
	// input bus //
	spi_mosi,
	spi_sclk,
	spi_ss,

	// output bus //
	sr_data,
	sr_clock,
	sr_latch,
	sr_enable,
	fet_gate
	);


	/** define parameter **/
	parameter ROW_BUS_WIDTH = 14;  // bit
	parameter ROW_COUNT_WIDTH = 4;  // bit
	parameter COL_BUS_WIDTH = 16;  // bit
	parameter COL_COUNT_WIDTH = 5;  // bit
	parameter BIT_COUNT_WIDTH = 3;  // bit

	parameter SR_EN_ASSERT_LEVEL = 0;


	/** include **/
	`include "state_2bit.vh"


	/** define pin **/
	// input bus //
	input spi_mosi;
	input spi_sclk;
	input spi_ss;

	// output bus //
	output sr_data;
	output sr_clock;
	output sr_latch;
	output sr_enable;
	output [ROW_BUS_WIDTH-1:0] fet_gate;


	/** define variable **/
	// counter //
	reg [BIT_COUNT_WIDTH-1:0] bit_count;
	reg [ROW_COUNT_WIDTH-1:0] row_count;
	reg [COL_COUNT_WIDTH-1:0] col_count;

	// flag //
	reg sr_clock_enableFlag;
	reg sr_latch_enableFlag;
	reg sr_enable_enableFlag;


	/** initialize **/
	initial begin
		// variable //
		bit_count <= 0;
		row_count <= 0;
		col_count <= 0;

		sr_clock_enableFlag <= 1;
		sr_latch_enableFlag <= 0;
		sr_enable_enableFlag <= ~SR_EN_ASSERT_LEVEL;
	end


	/** main logic **/
	always @( posedge spi_sclk or posedge spi_ss ) begin
		if ( spi_ss == 1 ) begin
			// spi end config
			bit_count <= 0;
			row_count <= 0;
			col_count <= 0;

			sr_clock_enableFlag <= 1;
			sr_latch_enableFlag <= 0;
			sr_enable_enableFlag <= ~SR_EN_ASSERT_LEVEL;
			
			state <= 0;
		end
		else begin
			case ( state )
				S0 : begin  // spi start config
					bit_count <= 0;
					row_count <= 0;
					col_count <= 0;

					state <= state + 1;
				end

				S1 : begin  // send data to shift registor
					if ( col_count == COL_BUS_WIDTH-1 ) begin
						col_count <= 0;

						sr_clock_enableFlag <= 0;
						sr_latch_enableFlag <= 1;
//						sr_enable_enableFlag <= ~SR_EN_ASSERT_LEVEL;

						state <= state + 1;
					end
					else begin
						col_count <= col_count + 1;
					end
				end

				S2 : begin // send latch clock to shift registor and enable output
//					sr_clock_enableFlag <= 0;
					sr_latch_enableFlag <= 0;
					sr_enable_enableFlag <= SR_EN_ASSERT_LEVEL;

					state <= state + 1;
				end

				S3 : begin  // padding 8bit, after shift registor output disable, line select
					if (bit_count == 6) begin
						bit_count <= 0;

						if (row_count == ROW_BUS_WIDTH-1) begin
							row_count <= 0;
						end
						else begin
							row_count <= row_count + 1;
						end

						sr_clock_enableFlag <= 1;
//						sr_latch_enableFlag <= 0;
						sr_enable_enableFlag <= ~SR_EN_ASSERT_LEVEL;

						state <= S1;
					end
					else begin
						bit_count <= bit_count + 1;
					end
				end

				default : begin
					state <= S0;
				end
			endcase
		end
	end


	/** connect port **/
	assign sr_data = spi_mosi & ~spi_ss;
	assign sr_clock = spi_sclk & sr_clock_enableFlag & ~spi_ss;
	assign sr_latch = spi_sclk & sr_latch_enableFlag;
	assign sr_enable = sr_enable_enableFlag;
	assign fet_gate = ~(1 << row_count);
endmodule
