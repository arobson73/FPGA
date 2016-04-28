/**************************************************************************
 * Copyright(c)2007 Altera Corporation, San Jose, California, USA.        *
 * All rights reserved. All use of this software and documentation is     *
 * subject to the License Agreement located at the end of this file below.*
 **************************************************************************
 * Author - YWGAN                                                         *
 * Date - 12/21/2007                                                      *
 *                                                                        *
 * This design example shows the use of DMA read-write masters with       *
 * hardware filter block incorperated to peform low pass filtering        *
 * operation on a set of incoming data. The performance of the            *
 * hardware filter is compared with the software version of the           *
 * filter. Both filter use 16 delay taps with same coefficients.          *
 *                                                                        *
 * Beside filtering operation, the masters can be incorperated with       *
 * any tranform block to perform certain operation. The driver for        *
 * accessing the masters is included in hw_fir.c file and the ways        *
 * of using it is the same no matter what transform block in used         *
 * between the master.                                                    *
 *************************************************************************/
//#undef ALT_ENHANCED_INTERRUPT_API_PRESENT

#include <stdio.h>
#include "stdlib.h"
#include "io.h"
#include "system.h"
#include "sys/alt_irq.h"
#include "alt_types.h"
#include "sys/alt_cache.h"
#include "hw_fir.h"

#define BUFFER_SIZE 128
#define IRQ 1
int main()
{
  int i, mismatch;
  int * source_buffer;
  int * dest_buffer;
  source_buffer = (int*) ONCHIP_MEMORY1_BASE;
 if (source_buffer == NULL)
 {
	 printf("Failed to allocate source buffer\n");
	 return 1;
 }
 printf("Address of source %p \n",source_buffer);

  dest_buffer = (int*) ONCHIP_MEMORY2_BASE;

 printf("Address of destinaion buffer %p \n",dest_buffer);
 
 
  
/**************************************************
 * Populating test data                           *
 * Zeroes are stuffed in to upscale the test data *
 *************************************************/           
  printf("Writing test data\n\n");
  for(i=0; i<(BUFFER_SIZE / 4); i++)
  {
        if((i%4)==0)
        {
            source_buffer[i] = i%(0x8000);  // keeps the input within the signed 16 bit domain
        }
        else
        {
            source_buffer[i] = 0;
        }
  }
/***************************************************/



/****************************************************
 * Start the DMA transfer to peform hardware filter *
 * operation. The operation is completed when the   *
 * DMA fires an interrupt or the DONE bit is high.  *
 ****************************************************/        
  printf("Starting dma operation\n");  
  if(dma_fir(source_buffer, dest_buffer, BUFFER_SIZE, IRQ))
  {    
    printf("Hardware filter failed, exiting...\n");     
    return 1;
  }
  else
  {
    printf("Hardware filter is done\n\n");    
  }   
/****************************************************/


/*****************************************************
 * Validates the results for both filter operations. *
 * Print the processing time taken for both filters  *
 * if all data match.                                *
 *****************************************************/      
  printf("Validating the results\n\n");
  i = 0;  // first eight outputs are random data and will differ if you re-run the software
  mismatch = 0;
  do  
  {
    if(source_buffer[i] != dest_buffer[i] )
    {
       mismatch = 1;
       printf("%i\n",i);
       printf("%i\n",source_buffer[i]);
       printf("%i\n",dest_buffer[i]);
    }
    i++;         
  } while((mismatch==0) && (i<(BUFFER_SIZE / 4)));

  return 0;
}

