#ifndef _DMA_AVALON_ACCESS_H_
#define _DMA_AVALON_ACCESS_H_

#include "io.h"

/* bit masks */
#define DMA_DONE_BIT        0x1
#define DMA_BUSY_BIT        0x2
#define DMA_CLR_BIT         0x1
#define DMA_GO_BIT          0x8
#define DMA_IRQ_E_BIT       0x10


/* register byte offsets */
#define DMA_STATUS_REG    	0
#define DMA_READ_ADDR_REG   4
#define DMA_WRITE_ADDR_REG  8
#define DMA_LENGTH_REG   	12
#define DMA_CTRL_REG   		24

/* register access macros */
#define DMA_WR_STATUS(base, data)   	IOWR_32DIRECT(base, DMA_STATUS_REG, data)
#define DMA_WR_READADDR(base, data)   	IOWR_32DIRECT(base, DMA_READ_ADDR_REG, data)
#define DMA_WR_WRITEADDR(base, data)   	IOWR_32DIRECT(base, DMA_WRITE_ADDR_REG, data)
#define DMA_WR_LEN(base, data)   		IOWR_32DIRECT(base, DMA_LENGTH_REG, data)
#define DMA_WR_CTRL(base, data)   		IOWR_32DIRECT(base, DMA_CTRL_REG, data)
#define DMA_RD_STATUS(base)   			IORD_32DIRECT(base, DMA_STATUS_REG)

#endif 
