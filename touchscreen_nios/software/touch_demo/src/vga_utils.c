#include "../inc/vga_utils.h"
#include "../inc/globals.h"
#include "../inc/fonts.h"

#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"

#include <stdio.h>
#include <stdbool.h>

// Public Functions
void drawEmptyBox(int x1, int y1, int x2, int y2, int color, int backbuffer);
void drawFilledBox(int x1, int y1, int x2, int y2, int color, int backbuffer);
void drawLine(int x1, int y1, int x2, int y2, int color, int backbuffer);
void drawVline(int x, int y1, int y2, int color, int backbuffer);
void drawHline(int x1, int x2, int y, int color, int backbuffer);
void clearScreen(int backbuffer);
void clearCharacters(void);
void drawStringPixels(const char * string, int x, int y, int color);
void drawString(const char* string, int x, int y);
void drawChar(char c, int x, int y);

// Private Functions
static int custom_alt_up_pixel_buffer_dma_draw(alt_up_pixel_buffer_dma_dev *pixel_buffer, unsigned int color, unsigned int x, unsigned int y);
static int custom_alt_up_pixel_buffer_dma_draw_clear(alt_up_pixel_buffer_dma_dev *pixel_buffer, unsigned int color, unsigned int x, unsigned int y);
static void OutGraphicsCharFont1(int x, int y, int fontcolour, int backgroundcolour, int c, int Erase);
static void OutGraphicsCharFont2(int x, int y, int fontcolour, int backgroundcolour, int c, int Erase);
static void waitForVGA(void);

void drawEmptyBox(int x1, int y1, int x2, int y2, int color, int backbuffer) {
  waitForVGA();

  drawVline(x1, y1, y2, color, backbuffer); //left
  drawVline(x2, y1, y2, color, backbuffer); //right
  drawHline(x1, x2, y1, color, backbuffer); //top
  drawHline(x1, x2, y2, color, backbuffer); //bottom
}

void drawFilledBox(int x1, int y1, int x2, int y2, int color, int backbuffer) {
  waitForVGA();

  for (int i = y1; i <= y2; i++) {
    for (int j = x1; j <= x2; j++) {
      custom_alt_up_pixel_buffer_dma_draw(g_pixel_buf_dma_dev, color, j, i);
    }
  }
}

void drawLine(int x1, int y1, int x2, int y2, int color, int backbuffer) {
  waitForVGA();
  alt_up_pixel_buffer_dma_draw_line(g_pixel_buf_dma_dev, x1, y1, x2, y2, color, backbuffer);
}

void drawVline(int x, int y1, int y2, int color, int backbuffer) {
  waitForVGA();
  
  for (int i = y1; i <= y2; i++) {
    custom_alt_up_pixel_buffer_dma_draw(g_pixel_buf_dma_dev, color, x, i);
  }
}

void drawHline(int x1, int x2, int y, int color, int backbuffer) {
  waitForVGA();

  for (int i = x1; i <= x2; i++) {
    custom_alt_up_pixel_buffer_dma_draw(g_pixel_buf_dma_dev, color, i, y);
  }
}

void clearScreen(int backbuffer) {
  for (int i = 0; i < 480; i += 1) {
    for (int j = 0; j < 640; j += 2) {
      custom_alt_up_pixel_buffer_dma_draw_clear(g_pixel_buf_dma_dev, BLACK, j, i);
    }
  }

  clearCharacters();

  waitForVGA();
}

void clearCharacters(void) {
	alt_up_char_buffer_clear(g_char_buf_dma_dev);
}

void drawStringPixels(const char * string, int x, int y, int color) {
	x = x-1;
  int i = 0;

  while (string[i] != '\0') {
    OutGraphicsCharFont2(x + i*10 + 2, y, color, BLACK, string[i], 0);
    i++;
  }
}

void drawString(const char * string, int x, int y) {
  float x_f = x * (float)CHAR_BUF_X/SCREEN_X - (int)(x * (float)CHAR_BUF_X/SCREEN_X);
  if (x_f > 0.5) {
    x = (int)(x * (float)CHAR_BUF_X/SCREEN_X) + 1;
  } else {
    x = (int)(x * (float)CHAR_BUF_X/SCREEN_X);
  }

  float y_f = y * (float)CHAR_BUF_Y/SCREEN_Y - (int)(y * (float)CHAR_BUF_Y/SCREEN_Y);
  if (y_f > 0.5) {
    y = (int)(y * (float)CHAR_BUF_Y/SCREEN_Y) + 1;
  } else {
    y = (int)(y * (float)CHAR_BUF_Y/SCREEN_Y);
  }

  alt_up_char_buffer_string(g_char_buf_dma_dev, string, x, y);  
}

void drawChar(char c, int x, int y) {
	float x_f = x * (float)CHAR_BUF_X/SCREEN_X - (int)(x * (float)CHAR_BUF_X/SCREEN_X);
  if (x_f > 0.5) {
    x = (int)(x * (float)CHAR_BUF_X/SCREEN_X) + 1;
  } else {
    x = (int)(x * (float)CHAR_BUF_X/SCREEN_X);
  }

  float y_f = y * (float)CHAR_BUF_Y/SCREEN_Y - (int)(y * (float)CHAR_BUF_Y/SCREEN_Y);
  if (y_f > 0.5) {
    y = (int)(y * (float)CHAR_BUF_Y/SCREEN_Y) + 1;
  } else {
    y = (int)(y * (float)CHAR_BUF_Y/SCREEN_Y);
  }

	alt_up_char_buffer_draw(g_char_buf_dma_dev, c, x, y);
}

static int custom_alt_up_pixel_buffer_dma_draw(alt_up_pixel_buffer_dma_dev *pixel_buffer, unsigned int color, unsigned int x, unsigned int y)
/* This function draws a pixel to the front buffer.
 */
{
	// boundary check
	if (x >= pixel_buffer->x_resolution || y >= pixel_buffer->y_resolution )
		return -1;

	unsigned int addr = 0;
	/* Check the mode VGA Pixel Buffer is using. */
	if (pixel_buffer->addressing_mode == ALT_UP_PIXEL_BUFFER_XY_ADDRESS_MODE) {
		/* For X-Y addressing mode, the address format is | unused | Y | X |. So shift bits for coordinates X and Y into their respective locations. */
		addr += ((x & pixel_buffer->x_coord_mask) << pixel_buffer->x_coord_offset);
		addr += ((y & pixel_buffer->y_coord_mask) << pixel_buffer->y_coord_offset);
	} else {
		/* In a consecutive addressing mode, the pixels are stored in consecutive memory locations. So the address of a pixel at (x,y) can be computed as
		 * (y*x_resolution + x).*/
		addr += ((x & pixel_buffer->x_coord_mask) << pixel_buffer->x_coord_offset);
		addr += (((y & pixel_buffer->y_coord_mask) * pixel_buffer->x_resolution) << pixel_buffer->x_coord_offset);
	}
	/* Now, depending on the color depth, write the pixel color to the specified memory location. */
	if (pixel_buffer->color_mode == ALT_UP_8BIT_COLOR_MODE) {
		IOWR_8DIRECT(pixel_buffer->buffer_start_address, addr, color);
	} else if (pixel_buffer->color_mode == ALT_UP_16BIT_COLOR_MODE) {
		IOWR_16DIRECT(pixel_buffer->buffer_start_address, addr, color);
	} else {
		IOWR_32DIRECT(pixel_buffer->buffer_start_address, addr, color);
	}

  waitForVGA();

	return 0;
}

static int custom_alt_up_pixel_buffer_dma_draw_clear(alt_up_pixel_buffer_dma_dev *pixel_buffer, unsigned int color, unsigned int x, unsigned int y)
/* This function draws a pixel to the front buffer.
 */
{
	// boundary check
	if (x >= pixel_buffer->x_resolution || y >= pixel_buffer->y_resolution )
		return -1;

	unsigned int addr = 0;
	/* Check the mode VGA Pixel Buffer is using. */
	if (pixel_buffer->addressing_mode == ALT_UP_PIXEL_BUFFER_XY_ADDRESS_MODE) {
		/* For X-Y addressing mode, the address format is | unused | Y | X |. So shift bits for coordinates X and Y into their respective locations. */
		addr += ((x & pixel_buffer->x_coord_mask) << pixel_buffer->x_coord_offset);
		addr += ((y & pixel_buffer->y_coord_mask) << pixel_buffer->y_coord_offset);
	} else {
		/* In a consecutive addressing mode, the pixels are stored in consecutive memory locations. So the address of a pixel at (x,y) can be computed as
		 * (y*x_resolution + x).*/
		addr += ((x & pixel_buffer->x_coord_mask) << pixel_buffer->x_coord_offset);
		addr += (((y & pixel_buffer->y_coord_mask) * pixel_buffer->x_resolution) << pixel_buffer->x_coord_offset);
	}

  IOWR_32DIRECT(pixel_buffer->buffer_start_address, addr, color);
	return 0;
}

static void waitForVGA(void) {
  while (alt_up_pixel_buffer_dma_check_swap_buffers_status(g_pixel_buf_dma_dev)) {
    printf("Waiting for drawing\n");
  }
}


static void OutGraphicsCharFont1(int x, int y, int fontcolour, int backgroundcolour, int c, int Erase)
{
  // using register variables (as opposed to stack based ones) may make execution faster
  // depends on compiler and CPU

	register int row, column, theX = x, theY = y ;
	register int pixels ;
	register char theColour = fontcolour  ;
	register int BitMask, theC = c ;

  // if x,y coord off edge of screen don't bother
  if(((short)(x) > (short)(SCREEN_X-1)) || ((short)(y) > (short)(SCREEN_Y-1)))
      return ;

  // if printable character subtract hex 20
	if(((short)(theC) >= (short)(' ')) && ((short)(theC) <= (short)('~'))) {
		theC = theC - 0x20 ;
		for(row = 0; (char)(row) < (char)(7); row ++)	{

      // get the bit pattern for row 0 of the character from the software font
			pixels = Font5x7[theC][row] ;
			BitMask = 16 ;

			for(column = 0; (char)(column) < (char)(5); column ++)	{

        // if a pixel in the character display it
				if((pixels & BitMask))
					// WriteAPixel(theX+column, theY+row, theColour) ;
          custom_alt_up_pixel_buffer_dma_draw(g_pixel_buf_dma_dev, theColour, theX+column, theY+row);

				else {
					if(Erase == true)

          custom_alt_up_pixel_buffer_dma_draw(g_pixel_buf_dma_dev, backgroundcolour, theX+column, theY+row);
				}

				BitMask = BitMask >> 1 ;
			}
		}
	}
}

static void OutGraphicsCharFont2(int x, int y, int colour, int backgroundcolour, int c, int Erase)
{
	register int 	row,
					column,
					theX = x,
					theY = y ;
	register int 	pixels ;
	register char 	theColour = colour  ;
	register int 	BitMask,
					theCharacter = c,
					j,
					theRow, theColumn;


    if(((short)(x) > (short)(SCREEN_X-1)) || ((short)(y) > (short)(SCREEN_Y-1)))  // if start off edge of screen don't bother
        return ;

	if(((short)(theCharacter) >= (short)(' ')) && ((short)(theCharacter) <= (short)('~'))) {			// if printable character
		theCharacter -= 0x20 ;																			// subtract hex 20 to get index of first printable character (the space character)
		theRow = 14;
		theColumn = 10;

		for(row = 0; row < theRow ; row ++)	{
			pixels = Font10x14[theCharacter][row] ;		     								// get the pixels for row 0 of the character to be displayed
			BitMask = 512 ;							   											// set of hex 200 i.e. bit 7-0 = 0010 0000 0000
			for(column = 0; column < theColumn;   )  	{
				if((pixels & BitMask))														// if valid pixel, then write it
					// WriteAPixel(theX+column, theY+row, theColour) ;
          custom_alt_up_pixel_buffer_dma_draw(g_pixel_buf_dma_dev, theColour, theX+column, theY+row);
				else {																		// if not a valid pixel, do we erase or leave it along (no erase)
					if(Erase == true)
						// WriteAPixel(theX+column, theY+row, backgroundcolour) ;
            custom_alt_up_pixel_buffer_dma_draw(g_pixel_buf_dma_dev, backgroundcolour, theX+column, theY+row);
					// else leave it alone
				}
					column ++ ;
				BitMask = BitMask >> 1 ;
			}
		}
	}
}


