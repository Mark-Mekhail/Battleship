#ifndef MAIN_H
#define MAIN_H

#include "system.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define WRITE_FIFO_EMPTY 128
#define NEW_LINE 0x3e

void drawBoxes(alt_up_pixel_buffer_dma_dev *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif