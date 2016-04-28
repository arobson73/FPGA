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
	Date:  11/04/2007
	
	This is the transfer block that joins the FIFO interfaces for the read and write master
	blocks.  This file simply connects the FIFO data together and provides flow control to
	ensure no data is lost.
	
	The transfer block used in this DMA can be replaced with any transformation logic.  You
	must simply ensure that the transfom block does not read from the read master FIFO when it
	is empty or write to the write master FIFO when it is full.  The interface for the transform
	block is as follows:
	
	Read Master			Transform Block			Write Master
	
	transform_readdata --------->
	transform_data_available --->
	transform_read <-------------
	
								----------> transform_writedata
								----------> transform_write
								<---------- transform_wait
								
	When transform_data_available is asserted and transform_wait is de-asserted then the transfer
	of data is performed.  This condition is used to drive the transform_read and transform_write
	signals.	
*/


module transform_block (
		clk,
		reset,
		transform_read,
		transform_readdata,
		transform_data_available,
		transform_write,
		transform_writedata,
		transform_wait,
		transform_clear
);


	parameter DATAWIDTH = 32;

	// interface signals
	input clk;
	input reset;
	input [DATAWIDTH-1:0] transform_readdata;
	input transform_data_available;
	input transform_wait;
	output wire transform_read;
	output wire [DATAWIDTH-1:0] transform_writedata;
	output wire transform_write;
	input transform_clear;
	
		
	// internal signals
	wire sink_ready;
	wire source_valid;
	wire [15:0] filter_output;


	//peform data transfer to FIR when there is valid sink data and the FIR is ready
	assign transform_read = (transform_data_available==1) & (sink_ready==1);

	//perform data transfer from FIR when there is valid source data and the downstream module is ready
	assign transform_write = (source_valid==1) & (transform_wait==0);

    //converting the 16 bits FIR output data to 32 bits
	assign transform_writedata = {16'b0, filter_output};

	custom_simple the_custom_simple (
		.clk (clk),
		.reset (reset),
		.clear (transform_clear),
		.sink_data (transform_readdata[15:0]),
		.sink_valid (transform_data_available),
		.sink_ready (sink_ready),
		.source_data (filter_output),
		.source_valid (source_valid),
		.source_ready(~transform_wait)
	);

	defparam the_custom_simple.DATA_WIDTH = 16;
	defparam the_custom_simple.FIFO_DEPTH = 32;  

endmodule
