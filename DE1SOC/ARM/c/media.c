#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "hwlib.h"
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "hps_0.h"
#define SDRAM_BASE1 0xC0000000
#define FPGA_CHAR_BASE        0xC9000000
#define FPGA_ONCHIP_BASE      0xC8000000

//#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_BASE ( SDRAM_BASE1 )
//#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_SPAN ( 0x40000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

/* globals */
#define BUF_SIZE 500000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96		// 75% of 128 word buffer

/* function prototypes */
void VGA_text (int, int, char *);
void VGA_box (int, int, int, int, short);
void HEX_PS2(char, char, char);
void check_KEYs( int *, int *, int * );

/*virtual base pointer*/
void *virtual_base;

/********************************************************************************
 * This program demonstrates use of the media ports in the DE1-SoC Computer
 *
 * It performs the following: 
 *  	1. records audio for about 10 seconds when KEY[0] is pressed. LEDR[0] is
 *  	   lit while recording
 * 	2. plays the recorded audio when KEY[1] is pressed. LEDR[1] is lit while 
 * 	   playing
 * 	3. Draws a blue box on the VGA display, and places a text string inside
 * 	   the box
 * 	4. Displays the last three bytes of data received from the PS/2 port 
 * 	   on the HEX displays on the DE1-SoC Computer
 ********************************************************************************/
int main(void)
{
	/* Declare volatile pointers to I/O registers (volatile means that IO load
	   and store instructions will be used to access these pointer locations, 
	   instead of regular memory loads and stores) */
	volatile unsigned int * red_LED_ptr = NULL;
	volatile unsigned int * audio_ptr   = NULL;
	volatile unsigned int * PS2_ptr     = NULL;
	int fd;
	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}
	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );	
	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return(1);
	}
	red_LED_ptr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + LEDS_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	audio_ptr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + AUDIO_SUBSYSTEM_AUDIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );

	PS2_ptr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + PS2_PORT_BASE ) & ( unsigned long)( HW_REGS_MASK ) );


	/* used for audio record/playback */
	int fifospace;
	int record = 0, play = 0, buffer_index = 0;
	int left_buffer[BUF_SIZE];
	int right_buffer[BUF_SIZE];

	/* used for PS/2 port data */
	int PS2_data, RVALID;
	char byte1 = 0, byte2 = 0, byte3 = 0;

	/* create a message to be displayed on the VGA and LCD displays */
	char text_top_row[40] = "Altera DE1-SoC\0";
	char text_bottom_row[40] = "Computer\0";

	VGA_text (34, 29, text_top_row);
	VGA_text (34, 30, text_bottom_row);
	VGA_box (0, 0, 319, 239, 0);						// clear the screen
	VGA_box (33*4, 28*4, 49*4, 32*4, 0x187F);

	/* read and echo audio data */
	record = 0;
	play = 0;

	// PS/2 mouse needs to be reset (must be already plugged in)
	*(PS2_ptr) = 0xFF;		// reset
	while(1)
	{
		check_KEYs ( &record, &play, &buffer_index );
		if (record)
		{
			*(red_LED_ptr) = 0x1;					// turn on LEDR[0]
			fifospace = *(audio_ptr + 1);	 		// read the audio port fifospace register
			if ( (fifospace & 0x000000FF) > BUF_THRESHOLD ) 	// check RARC
			{
				// store data until the the audio-in FIFO is empty or the buffer is full
				while ( (fifospace & 0x000000FF) && (buffer_index < BUF_SIZE) )
				{
					left_buffer[buffer_index] = *(audio_ptr + 2); 		
					right_buffer[buffer_index] = *(audio_ptr + 3); 		
					++buffer_index;

					if (buffer_index == BUF_SIZE)
					{
						// done recording
						record = 0;
						*(red_LED_ptr) = 0x0;				// turn off LEDR
					}
					fifospace = *(audio_ptr + 1);	// read the audio port fifospace register
				}
			}
		}
		else if (play)
		{
			*(red_LED_ptr) = 0x2;					// turn on LEDR_1
			fifospace = *(audio_ptr + 1);	 		// read the audio port fifospace register
			if ( (fifospace & 0x00FF0000) > BUF_THRESHOLD ) 	// check WSRC
			{
				// output data until the buffer is empty or the audio-out FIFO is full
				while ( (fifospace & 0x00FF0000) && (buffer_index < BUF_SIZE) )
				{
					*(audio_ptr + 2) = left_buffer[buffer_index];
					*(audio_ptr + 3) = right_buffer[buffer_index];
					++buffer_index;

					if (buffer_index == BUF_SIZE)
					{
						// done playback
						play = 0;
						*(red_LED_ptr) = 0x0;				// turn off LEDR
					}
					fifospace = *(audio_ptr + 1);	// read the audio port fifospace register
				}
			}
		}
		/* check for PS/2 data--display on HEX displays */
		PS2_data = *(PS2_ptr);			// read the Data register in the PS/2 port
		RVALID = PS2_data & 0x8000;	// extract the RVALID field
		if (RVALID)
		{
			/* shift the next data byte into the display */
			byte1 = byte2;
			byte2 = byte3;
			byte3 = PS2_data & 0xFF;
			HEX_PS2 (byte1, byte2, byte3);

			if ( (byte2 == (char) 0xAA) && (byte3 == (char) 0x00) )
				// mouse inserted; initialize sending of data
				*(PS2_ptr) = 0xF4;
		}
	}
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );

	}
	close( fd );
	return 0;	
}

/****************************************************************************************
 * Subroutine to send a string of text to the VGA monitor 
 ****************************************************************************************/
void VGA_text(int x, int y, char * text_ptr)
{
	int offset;
	//	volatile char * character_buffer = (char *) FPGA_CHAR_BASE;	// VGA character buffer
	volatile char * character_buffer =NULL;
	character_buffer =(char*) (virtual_base + ( ( unsigned long  )( FPGA_CHAR_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
	// VGA character buffer

	/* assume that the text string fits on one line */
	offset = (y << 7) + x;
	while ( *(text_ptr) )
	{
		*(character_buffer + offset) = *(text_ptr);	// write to the character buffer
		++text_ptr;
		++offset;
	}
}

/****************************************************************************************
 * Draw a filled rectangle on the VGA monitor 
 ****************************************************************************************/
void VGA_box(int x1, int y1, int x2, int y2, short pixel_color)
{
	int pixel_ptr, row, col;


	/* assume that the box coordinates are valid */
	for (row = y1; row <= y2; row++)
		for (col = x1; col <= x2; ++col)
		{
			pixel_ptr = (int)(virtual_base + ( ( unsigned long  )( FPGA_ONCHIP_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));

			pixel_ptr = pixel_ptr | (row << 10) | (col << 1);
			*(short *)pixel_ptr = pixel_color;		// set pixel color
		}
}

/****************************************************************************************
 * Subroutine to show a string of HEX data on the HEX displays
 ****************************************************************************************/
void HEX_PS2(char b1, char b2, char b3)
{
	volatile unsigned int * HEX3_HEX0_ptr = NULL;
	volatile unsigned int * HEX5_HEX4_ptr = NULL;

	HEX3_HEX0_ptr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + HEX3_HEX0_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	HEX5_HEX4_ptr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + HEX5_HEX4_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	/* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in 
	 * a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F */
	unsigned char	seven_seg_decode_table[] = {	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7C, 0x07, 
		0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };
	unsigned char	hex_segs[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned int shift_buffer, nibble;
	unsigned char code;
	int i;

	shift_buffer = (b1 << 16) | (b2 << 8) | b3;
	for ( i = 0; i < 6; ++i )
	{
		nibble = shift_buffer & 0x0000000F;		// character is in rightmost nibble
		code = seven_seg_decode_table[nibble];
		hex_segs[i] = code;
		shift_buffer = shift_buffer >> 4;
	}
	/* drive the hex displays */
	*(HEX3_HEX0_ptr) = *(int *) (hex_segs);
	*(HEX5_HEX4_ptr) = *(int *) (hex_segs+4);
}

/****************************************************************************************
 * Subroutine to read KEYs
 ****************************************************************************************/
void check_KEYs(int * KEY0, int * KEY1, int * counter)
{
	volatile unsigned int * KEY_ptr = NULL;
	volatile unsigned int * audio_ptr = NULL;
	int KEY_value;

	KEY_ptr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + PUSHBUTTONS_BASE ) & ( unsigned long)( HW_REGS_MASK ) );

	audio_ptr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + AUDIO_SUBSYSTEM_AUDIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	KEY_value = *(KEY_ptr); 				// read the pushbutton KEY values
	while (*KEY_ptr);							// wait for pushbutton KEY release

	if (KEY_value == 0x1)					// check KEY0
	{
		// reset counter to start recording
		*counter = 0;
		// clear audio-in FIFO
		*(audio_ptr) = 0x4;
		*(audio_ptr) = 0x0;

		*KEY0 = 1;
	}
	else if (KEY_value == 0x2)				// check KEY1
	{
		// reset counter to start playback
		*counter = 0;
		// clear audio-out FIFO
		*(audio_ptr) = 0x8;
		*(audio_ptr) = 0x0;

		*KEY1 = 1;
	}
}
