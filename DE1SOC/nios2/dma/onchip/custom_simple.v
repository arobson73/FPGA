// altera verilog_input_version SystemVerilog_2005

/*
  Legal Notice: (C)2007 Altera Corporation. All rights reserved.  Your
  use of Altera Corporation's design tools, logic functions and other
  software and tools, and its AMPP partner logic functions, and any
  output files any of the foregoing (including device programming or
  simulation files), and any associated documentation or information are
  expressly subject to the terms and conditions of the Altera Program
  License Subscription Agreement or other applicable license agreement,
  including, without limitation, that your use is for the sole purpose
  of programming logic devices manufactured by Altera and sold by Altera
  or its authorized distributors.  Please refer to the applicable
  agreement for further details.
*/

/*

	Author:  JCJB
	Date:  01/28/2008
	
	This block contains a FIR filter capable of the following:
	
	- 4/8/16 symetrical taps
	- parameterizable FIFO depth
	- Avalon-ST integration
	
	This hardware has not been optimized for targeting DSP blocks
	or memory.  It is recommended to replace the multipliers with
	memory multiplications.
	
	The register "tag" is used to perform flow control to avoid overflowing
	the output FIFO.  For every data sample that enters the filter, five clock
	cycles of latency will occur before the result is stuffed into the FIFO.
	If the amount of pipelining changes so must the tap width used to track valid
	data through the pipeline of adders and multipliers.  This filter supports
	clearing functionality as well if a new data stream is provided at the input.
*/

module custom_simple (
	clk,
	reset,

	// the "sink" side
	sink_data,
	sink_valid,
	sink_ready,

	// the "source" side
	source_data,
	source_valid,
	source_ready,
	
	clear
);

	parameter DATA_WIDTH = 16;  // input data width (output is wider)
	parameter FIFO_DEPTH = 32;  // make sure this is at least 32
        parameter FIFODEPTH_LOG2 = 5; //log2 of the FIFO_DEPTH

	input clk;
	input reset;
	

	// the "sink" side
	input signed [DATA_WIDTH-1:0] sink_data;
	input sink_valid;
	output wire sink_ready;


	// the "source" side
	output wire [(DATA_WIDTH-1):0] source_data;
	output wire source_valid;
	input source_ready;


	input clear;
	

	reg signed [DATA_WIDTH-1:0] in1 ,in2,in3,in4,in5;  

	reg [4:0] tag;  // shift register that tracks the valid data moving through the adders and multipliers


	// other misc. control signal
	wire read_fifo;
	wire pipeline_enable;
	wire fifo_empty;
	wire fifo_half_full;

/**********************************************************************************
 TAP PIPELINE
 **********************************************************************************/
	always @ (posedge clk or posedge reset)
	begin
		if (reset)
		begin
			in1 <= 0;
		end
		else if (clear == 1)
		begin
			in1 <= 0;
		end
		else if (pipeline_enable == 1)
		begin
			in1 <= sink_data;
		end
	end	
/**********************************************************************************/	


/**********************************************************************************
 INITIAL ADDER PIPELINE
 **********************************************************************************/
	always @ (posedge clk or posedge reset)
		begin
			if (reset)
			begin
				in2 <= 0;
			end
			else if (clear == 1)
			begin
				in2 <= 0;
			end
			else
			begin
				in2 <= in1;
			end
		end
/**********************************************************************************/



/**********************************************************************************
 MULTIPLIER PIPELINE
 **********************************************************************************/
	always @ (posedge clk or posedge reset)
		begin
			if (reset)
			begin
				in3 <= 0;
			end
			else if (clear == 1)
			begin
				in3 <= 0;
			end
			else
			begin
				in3 <= in2;
			end
		end
/**********************************************************************************/



/**********************************************************************************
 FINAL ADDER PIPELINE
 **********************************************************************************/
	always @ (posedge clk or posedge reset)
		begin
			if (reset)
			begin
				in4 <= 0;
			end
			else if (clear == 1)
			begin
				in4 <= 0;
			end
			else
			begin
				in4 <= in3;
			end
		end
/**********************************************************************************/



/**********************************************************************************
 FINAL RESULT PIPELINE
 **********************************************************************************/
	always @ (posedge clk or posedge reset)
	begin
		if (reset)
		begin
			in5 <= 0;
		end
		else if (clear == 1)
		begin
			in5 <= 0;
		end
		else
		begin
			in5 <= in4;
		end
	end
/**********************************************************************************/



/**********************************************************************************
 OUTPUT FIFO AND SYNC LOGIC (used for backpressure purposes)
 **********************************************************************************/
	assign sink_ready = (fifo_half_full == 0);
	assign pipeline_enable = (sink_ready == 1) & (sink_valid == 1);

	assign source_valid = (fifo_empty == 0);
	assign read_fifo = (source_valid == 1) & (source_ready == 1);

	always @ (posedge clk or posedge reset)
	begin
		if (reset)
		begin
			tag <= 0;
		end
		else if (clear == 1)
		begin
			tag <= 0;
		end
		else
		begin
			tag <= {tag[3:0], pipeline_enable}; 
		end
	end



	scfifo the_output_fifo (
		.aclr (reset),
		.sclr (clear),
		.clock (clk),
		.data (in5),
		.almost_full (fifo_half_full),
		.empty (fifo_empty),
		.q (source_data),
		.rdreq (read_fifo),
		.wrreq (tag[4])  // tag delay pipeline matches when valid data pops out of "final_result"
	);
	defparam the_output_fifo.LPM_WIDTH = DATA_WIDTH;
	defparam the_output_fifo.LPM_NUMWORDS = FIFO_DEPTH;
	
	defparam the_output_fifo.LPM_WIDTHU = FIFODEPTH_LOG2;
	
	defparam the_output_fifo.ALMOST_FULL_VALUE = FIFO_DEPTH - 2 - 6;  // -2 for the FIFO latency, and -6 for the pipelining latency of the FIR operation with room to spare
	defparam the_output_fifo.LPM_SHOWAHEAD = "ON";
	defparam the_output_fifo.USE_EAB = "ON";
	defparam the_output_fifo.ADD_RAM_OUTPUT_REGISTER = "OFF";
	defparam the_output_fifo.UNDERFLOW_CHECKING = "OFF";
	defparam the_output_fifo.OVERFLOW_CHECKING = "OFF";
/**********************************************************************************/


endmodule
