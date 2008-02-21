/*  Konami FireBeat

    Driver by Ville Linde



    Hardware overview:

    GQ972 PWB(A2) 0000070609 Main board
    -----------------------------------
        IBM PowerPC 403GCX at 66MHz
        (2x) Konami 0000057714 (2D object processor)
        Yamaha YMZ280B (ADPCM sound chip)
        Epson RTC65271 RTC/NVRAM
        National Semiconductor PC16552DV (dual UART)

    GQ974 PWB(A2) 0000070140 Extend board
    -------------------------------------
        ADC0808CCN
        FDC37C665GT (floppy disk controller)
        National Semiconductor PC16552DV (dual UART)
        ADM223LJR (RS-232 driver/receiver)

    GQ971 PWB(A2) 0000070254 SPU
    ----------------------------
        Motorola MC68HC000FN16 at 16MHz (?)
        Xilinx XC9572 CPLD
        Ricoh RF5c400 sound chip

    GQ971 PWB(B2) 0000067784 Backplane
    ----------------------------------
        3x PCB Slots with 2x DIN96S connectors (for main and extend PCBs)
        40-pin ATAPI connector for each slot

    GQ986 PWB(A1) 0000073015 Backplane
    ----------------------------------
        2x PCB Slots with 2x DIN96S connectors (for main and extend PCBs)
        40-pin ATAPI connector for each slot

    GQ972 PWB(D2) Controller interface on Beatmania III (?)
    GQ972 PWB(G1) Sound Amp (?)
    GQ971 PWB(C) Sound Amp


    Hardware configurations:

    Beatmania III
    -------------
        GQ971 Backplane
        GQ972 Main Board
        GQ974 Extend Board
        GQ971 SPU
        GQ972 Controller Interface
        GQ972 Sound Amp
        Hard drive in Slot 1
        DVD drive in Slot 2

    Keyboardmania
    -------------
        GQ971 Backplane
        GQ972 Main Board
        GQ974 Extend Board
        Yamaha XT446 board (for keyboard sounds) (the board layout matches the Yamaha MU100 Tone Generator)
        GQ971 Sound Amp
        CD-ROM drive in Slot 1
        CD-ROM drive in Slot 2

    ParaParaParadise
    ----------------
        GQ986 Backplane
        GQ972 Main Board
        2x CD-ROM drive in Slot 1



    Games that run on this hardware:

    BIOS       Game ID        Year    Game
    ------------------------------------------------------------------
    GQ972      GQ972          2000    Beatmania III
    GQ972(?)   ?              2001    Beatmania III Append 6th Mix
    GQ972(?)   ?              2002    Beatmania III Append 7th Mix
    GQ972(?)   ?              2000    Beatmania III Append Core Remix
    GQ972(?)   ?              2003    Beatmania III The Final
    GQ974      GQ974          2000    Keyboardmania
    GQ974      GCA01          2000    Keyboardmania 2nd Mix
    GQ974      GCA12          2001    Keyboardmania 3rd Mix
    GQ977      GQ977          2000    Para Para Paradise
    GQ977      GQ977          2000    Para Para Dancing
    GQ977      GC977          2000    Para Para Paradise 1.1
    GQ977      GQA11          2000    Para Para Paradise 1st Mix+
    GQA02(?)   ?              2000    Pop n' Music 4
    ???        ?              2000    Pop n' Music 5
    ???        ?              2001    Pop n' Music 6
    GQA02      GCB00          2001    Pop n' Music 7
    ???        ?              2002    Pop n' Music 8
    ???        ?              2000    Pop n' Music Animelo
    ???        ?              2001    Pop n' Music Animelo 2
    ???        ?              2001    Pop n' Music Mickey Tunes

    TODO:
        - The display list pointer setting in the graphics chip is not understood. Currently
          it has to be changed manually. (BIOS = 0x200000, PPP bootup = 0x1D0800, ingame = 0x0000, 0x8000 & 0x10000)

        - Keyboardmania 3rd mix is missing some graphics in the ingame. Most notably the backgrounds.
          The display list objects seem to be there, but the address is wrong (0x14c400, instead of the correct address)

        - The external Yamaha MIDI sound board is not emulated (no keyboard sounds).
*/

#include "driver.h"
#include "deprecat.h"
#include "cpu/powerpc/ppc.h"
#include "machine/intelfsh.h"
#include "machine/scsicd.h"
#include "machine/rtc65271.h"
#include "machine/pc16552d.h"
#include "sound/ymz280b.h"
#include "sound/cdda.h"
#include "cdrom.h"
#include "firebeat.lh"



typedef struct
{
	UINT32 *vram;
	UINT32 vram_read_address;
	UINT32 vram_write_fifo_address;
	UINT32 visible_area;
} GCU_REGS;

static UINT8 xram[4096];
static UINT8 extend_board_irq_enable;
static UINT8 extend_board_irq_active;

static emu_timer *keyboard_timer;

static GCU_REGS gcu[2];

static VIDEO_START(firebeat)
{
	gcu[0].vram = auto_malloc(0x2000000);
	gcu[1].vram = auto_malloc(0x2000000);
	memset(gcu[0].vram, 0, 0x2000000);
	memset(gcu[1].vram, 0, 0x2000000);
}


static void gcu_draw_object(int chip, mame_bitmap *bitmap, const rectangle *cliprect, UINT32 *cmd)
{
	// 0x00: xxx----- -------- -------- --------   command type
	// 0x00: -------- xxxxxxxx xxxxxxxx xxxxxxxx   object data address in vram

	// 0x01: -------- -------- ------xx xxxxxxxx   object x
	// 0x01: -------- ----xxxx xxxxxx-- --------   object y
	// 0x01: -----x-- -------- -------- --------   object x flip
	// 0x01: ----x--- -------- -------- --------   object y flip
	// 0x01: ---x---- -------- -------- --------   object alpha enable (?)

	// 0x02: -------- -------- ------xx xxxxxxxx   object width
	// 0x02: -------- -----xxx xxxxxx-- --------   object x scale

	// 0x03: -------- -------- ------xx xxxxxxxx   object height
	// 0x03: -------- -----xxx xxxxxx-- --------   object y scale

	int x				= cmd[1] & 0x3ff;
	int y				= (cmd[1] >> 10) & 0x3ff;
	int width			= (cmd[2] & 0x3ff) + 1;
	int height			= (cmd[3] & 0x3ff) + 1;
	int xscale			= (cmd[2] >> 10) & 0x1ff;
	int yscale			= (cmd[3] >> 10) & 0x1ff;
	int xflip			= (cmd[1] & 0x04000000) ? 1 : 0;
	int yflip			= (cmd[1] & 0x08000000) ? 1 : 0;
	int alpha_enable	= (cmd[1] & 0x10000000) ? 1 : 0;
	UINT32 address		= cmd[0] & 0xffffff;
	int alpha_level		= (cmd[2] >> 27) & 0x1f;

	int i, j;
	int u, v;
	UINT16 *vr = (UINT16*)gcu[chip].vram;

	if (xscale == 0 || yscale == 0)
	{
		xscale = 0x40;
		yscale = 0x40;
		return;
	}

	//if ((cmd[2] >> 24) != 0x84 && (cmd[2] >> 24) != 0x04 && (cmd[2] >> 24) != 0x00)
	//  printf("Unknown value = %d, %d\n", (cmd[2] >> 27) & 0x1f, (cmd[2] >> 22) & 0x1f);

	width	= (((width * 65536) / xscale) * 64) / 65536;
	height	= (((height * 65536) / yscale) * 64) / 65536;

	if (y > cliprect->max_y || x > cliprect->max_x) {
		return;
	}
	if ((y+height) > cliprect->max_y) {
		height = cliprect->max_y - y;
	}
	if ((x+width) > cliprect->max_x) {
		width = cliprect->max_x - x;
	}

	v = 0;
	for (j=0; j < height; j++)
	{
		int xi;
		int index;
		UINT16 *d = BITMAP_ADDR16(bitmap, j+y, x);
		//int index = address + ((v >> 6) * 1024);

		if (yflip)
		{
			index = address + ((height - 1 - (v >> 6)) * 1024);
		}
		else
		{
			index = address + ((v >> 6) * 1024);
		}

		if (xflip)
		{
			d += width;
			xi = -1;
		}
		else
		{
			xi = 1;
		}

		u = 0;
		for (i=0; i < width; i++)
		{
			UINT16 pix = vr[((index + (u >> 6)) ^ 1) & 0xffffff];

			if (alpha_enable)
			{
				if (pix & 0x8000)
				{
					if ((pix & 0x7fff) != 0)
					{
						//*d = pix & 0x7fff;
						UINT16 srcpix = *d;
						/*
                        UINT32 r = pix & 0x7c00;
                        UINT32 g = pix & 0x03e0;
                        UINT32 b = pix & 0x001f;

                        UINT32 sr = srcpix & 0x7c00;
                        UINT32 sg = srcpix & 0x03e0;
                        UINT32 sb = srcpix & 0x001f;

                        sr += r;
                        sg += g;
                        sb += b;
                        if (sr > 0x7c00) sr = 0x7c00;
                        if (sg > 0x03e0) sg = 0x03e0;
                        if (sb > 0x001f) sb = 0x001f;

                        *d = sr | sg | sb;
                        */

						UINT32 sr = (srcpix >> 10) & 0x1f;
						UINT32 sg = (srcpix >>  5) & 0x1f;
						UINT32 sb = (srcpix >>  0) & 0x1f;
						UINT32 r = (pix >> 10) & 0x1f;
						UINT32 g = (pix >>  5) & 0x1f;
						UINT32 b = (pix >>  0) & 0x1f;

						sr += (r * alpha_level) >> 4;
						sg += (g * alpha_level) >> 4;
						sb += (b * alpha_level) >> 4;

						if (sr > 0x1f) sr = 0x1f;
						if (sg > 0x1f) sg = 0x1f;
						if (sb > 0x1f) sb = 0x1f;

						*d = (sr << 10) | (sg << 5) | sb;
					}
				}
			}
			else
			{
				if (pix & 0x8000)
				{
					*d = pix & 0x7fff;
				}
			}

			if ((cmd[0] & 0x10000000) == 0)
				*d = 0x7fff;

			d += xi;
			u += xscale;
		}

		v += yscale;
	}
}

static void gcu_fill_rect(mame_bitmap *bitmap, const rectangle *cliprect, UINT32 *cmd)
{
	int i, j;
	int x1, y1, x2, y2;

	int x				= cmd[1] & 0x3ff;
	int y				= (cmd[1] >> 10) & 0x3ff;
	int width			= (cmd[0] & 0x3ff) + 1;
	int height			= ((cmd[0] >> 10) & 0x3ff) + 1;

	UINT16 color[4];

	color[0] = (cmd[2] >> 16);
	color[1] = (cmd[2] >>  0);
	color[2] = (cmd[3] >> 16);
	color[3] = (cmd[3] >>  0);

	x1 = x;
	x2 = x + width;
	y1 = y;
	y2 = y + height;

	if ((color[0] & 0x8000) == 0 && (color[1] & 0x8000) == 0 && (color[2] & 0x8000) == 0 && (color[3] & 0x8000) == 0)
	{
		// optimization, nothing to fill
		return;
	}

	// clip
	if (x1 < cliprect->min_x)	x1 = cliprect->min_x;
	if (y1 < cliprect->min_y)	y1 = cliprect->min_y;
	if (x2 > cliprect->max_x)	x2 = cliprect->max_x;
	if (y2 > cliprect->max_y)	y2 = cliprect->max_y;

	for (j=y1; j < y2; j++)
	{
		UINT16 *d = BITMAP_ADDR16(bitmap, j, 0);
		for (i=x1; i < x2; i++)
		{
			if (color[i&3] & 0x8000)
			{
				d[i] = color[i&3] & 0x7fff;
			}
		}
	}
}

static void gcu_draw_character(int chip, mame_bitmap *bitmap, const rectangle *cliprect, UINT32 *cmd)
{
	// 0x00: xxx----- -------- -------- --------   command type
	// 0x00: -------- xxxxxxxx xxxxxxxx xxxxxxxx   character data address in vram

	// 0x01: -------- -------- ------xx xxxxxxxx   character x
	// 0x01: -------- ----xxxx xxxxxx-- --------   character y

	// 0x02: xxxxxxxx xxxxxxxx -------- --------   color 0
	// 0x02: -------- -------- xxxxxxxx xxxxxxxx   color 1

	// 0x03: xxxxxxxx xxxxxxxx -------- --------   color 2
	// 0x03: -------- -------- xxxxxxxx xxxxxxxx   color 3

	int i, j;
	int x				= cmd[1] & 0x3ff;
	int y				= (cmd[1] >> 10) & 0x3ff;
	UINT32 address		= cmd[0] & 0xffffff;
	UINT16 color[4];

	UINT16 *vr = (UINT16*)gcu[chip].vram;

	color[0] = (cmd[2] >> 16) & 0xffff;
	color[1] = (cmd[2] >>  0) & 0xffff;
	color[2] = (cmd[3] >> 16) & 0xffff;
	color[3] = (cmd[3] >>  0) & 0xffff;


	if (y > cliprect->max_y || x > cliprect->max_x) {
		return;
	}


	for (j=0; j < 8; j++)
	{
		UINT16 *d = BITMAP_ADDR16(bitmap, y+j, x);
		UINT16 line = vr[address^1];

		address += 4;

		for (i=0; i < 8; i++)
		{
			int pix = (line >> ((7-i) * 2)) & 3;
			d[i] = color[pix];
		}
	}
}

static void gcu_exec_display_list(int chip, mame_bitmap *bitmap, const rectangle *cliprect, UINT32 address)
{
	int counter = 0;
	int end = 0;

	int i = address / 4;
	if (i < 0) i = 0;
	while (!end && counter < 0x1000 && i < (0x2000000/4))
	{
		int command;
		UINT32 cmd[4];
		cmd[0] = gcu[chip].vram[i+0];
		cmd[1] = gcu[chip].vram[i+1];
		cmd[2] = gcu[chip].vram[i+2];
		cmd[3] = gcu[chip].vram[i+3];

		command = (cmd[0] >> 29) & 0x7;

		switch (command)
		{
			case 0x0:		// ???
			{
				break;
			}

			case 0x1:		// Branch
			{
				gcu_exec_display_list(chip, bitmap, cliprect, cmd[0] & 0xffffff);
				break;
			}

			case 0x2:		// End of display list
			{
				end = 1;
				break;
			}

			case 0x3:		// ???
			{
				break;
			}

			case 0x4:		// Fill rectangle
			{
				gcu_fill_rect(bitmap, cliprect, cmd);
				break;
			}

			case 0x5:		// Draw object
			{
				gcu_draw_object(chip, bitmap, cliprect, cmd);
				break;
			}

			case 0x7:		// Draw 8x8 Character (2-bits per pixel)
			{
				gcu_draw_character(chip, bitmap, cliprect, cmd);
				break;
			}

			default:
				//printf("Unknown command %08X %08X %08X %08X at %08X\n", cmd[0], cmd[1], cmd[2], cmd[3], i*4);
				break;
		}

		i += 4;
		counter++;
	};
}

static int tick = 0;
static int layer = 0;
static VIDEO_UPDATE(firebeat)
{
	int chip = screen;
	//int i;

	fillbitmap(bitmap, 0, cliprect);

	if (mame_stricmp(machine->gamedrv->name, "popn7") == 0)
	{
		gcu_exec_display_list(chip, bitmap, cliprect, 0x1f80000);
	}
	else
	{
		if (layer >= 2)
		{
			gcu_exec_display_list(chip, bitmap, cliprect, 0x8000);
			gcu_exec_display_list(chip, bitmap, cliprect, 0x0000);
			gcu_exec_display_list(chip, bitmap, cliprect, 0x10000);
		}
		else if (layer == 0)
		{
			gcu_exec_display_list(chip, bitmap, cliprect, 0x200000);

			//gcu_exec_display_list(chip, bitmap, cliprect, 0x186040);
		}
		else if (layer == 1)
		{
			gcu_exec_display_list(chip, bitmap, cliprect, 0x1d0800);

			gcu_exec_display_list(chip, bitmap, cliprect, 0x1a9440);
		}
	}

	tick++;
	if (tick >= 5)
	{
		tick = 0;
		if (input_code_pressed(KEYCODE_0))
		{
			layer++;
			if (layer > 2)
			{
				layer = 0;
			}
		}

		/*
        if (input_code_pressed_once(KEYCODE_9))
        {
            FILE *file = fopen("vram0.bin", "wb");
            int i;

            for (i=0; i < 0x2000000/4; i++)
            {
                fputc((gcu[0].vram[i] >> 24) & 0xff, file);
                fputc((gcu[0].vram[i] >> 16) & 0xff, file);
                fputc((gcu[0].vram[i] >> 8) & 0xff, file);
                fputc((gcu[0].vram[i] >> 0) & 0xff, file);
            }

            fclose(file);
            file = fopen("vram1.bin", "wb");

            for (i=0; i < 0x2000000/4; i++)
            {
                fputc((gcu[1].vram[i] >> 24) & 0xff, file);
                fputc((gcu[1].vram[i] >> 16) & 0xff, file);
                fputc((gcu[1].vram[i] >> 8) & 0xff, file);
                fputc((gcu[1].vram[i] >> 0) & 0xff, file);
            }

            fclose(file);
        }
        */
	}

	return 0;
}

static UINT32 GCU_r(int chip, UINT32 offset, UINT32 mem_mask)
{
	int reg = offset * 4;

	/* VRAM Read */
	if (reg >= 0x80 && reg < 0x100)
	{
		return gcu[chip].vram[gcu[chip].vram_read_address + ((reg/4) - 0x20)];
	}

	switch(reg)
	{
		case 0x78:		/* GCU Status */
			return 0xffff0005;

		default:
			break;
	}

	return 0xffffffff;
}

static void GCU_w(int chip, UINT32 offset, UINT32 data, UINT32 mem_mask)
{
	int reg = offset * 4;

	if (reg != 0x70 && chip == 0)
	{
		//printf("gcu%d_w: %08X, %08X, %08X at %08X\n", chip, data, offset, mem_mask, activecpu_get_pc());
		//logerror("gcu%d_w: %08X, %08X, %08X at %08X\n", chip, data, offset, mem_mask, activecpu_get_pc());
	}

	switch(reg)
	{
		case 0x10:		/* ??? */
			break;

		case 0x30:
		//case 0x34:
		//case 0x38:
		//case 0x3c:
		{
			COMBINE_DATA( &gcu[chip].visible_area );
			if (ACCESSING_LSW32)
			{
				int numscreens = video_screen_count(Machine->config);
				int screen = chip;
				int width, height;
				screen_state *state = &Machine->screen[screen];
				rectangle visarea = state->visarea;

				width = (gcu[chip].visible_area & 0xffff);
				height = (gcu[chip].visible_area >> 16) & 0xffff;
				//set_visible_area(0, width, 0, height);

				visarea.max_x = width-1;
				visarea.max_y = height-1;

				// only try and update the screen if the driver says we have one
				if (screen < numscreens)
					video_screen_configure(screen, visarea.max_x + 1, visarea.max_y + 1, &visarea, Machine->screen[screen].refresh);
			}
			break;
		}

		case 0x40:		/* ??? */
			break;

		//case 0x44:    /* ??? */
		//  break;

		case 0x5c:		/* VRAM Read Address */
			gcu[chip].vram_read_address = (data & 0xffffff) / 2;
			break;

		case 0x60:		/* VRAM FIFO Write Address */
			gcu[chip].vram_write_fifo_address = (data & 0xffffff) / 2;

	//      printf("gcu%d_w: %08X, %08X, %08X\n", chip, data, offset, mem_mask);
			break;

		case 0x68:		/* Unknown */
		{
			break;
		}

		case 0x70:		/* VRAM FIFO Write */
			gcu[chip].vram[gcu[chip].vram_write_fifo_address] = data;
			gcu[chip].vram_write_fifo_address++;
			break;

		default:
	//      printf("gcu%d_w: %08X, %08X, %08X\n", chip, data, offset, mem_mask);
			break;
	}
}

static READ32_HANDLER(gcu0_r)
{
	return GCU_r(0, offset, mem_mask);
}

static WRITE32_HANDLER(gcu0_w)
{
	GCU_w(0, offset, data, mem_mask);
}

static READ32_HANDLER(gcu1_r)
{
	return GCU_r(1, offset, mem_mask);
}

static WRITE32_HANDLER(gcu1_w)
{
	GCU_w(1, offset, data, mem_mask);
}

/*****************************************************************************/

static READ32_HANDLER(input_r)
{
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= (readinputport(0) & 0xff) << 24;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= (readinputport(1) & 0xff) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= (readinputport(2) & 0xff);
	}

	return r;
}

static READ32_HANDLER( sensor_r )
{
	if (offset == 0)
	{
		return readinputport(3) | 0x01000100;
	}
	else
	{
		return readinputport(4) | 0x01000100;
	}
}

static READ32_HANDLER(flashram_r)
{
	UINT32 r = 0;
	if (!(mem_mask & 0xff000000))
	{
		r |= (intelflash_read(0, (offset*4)+0) & 0xff) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= (intelflash_read(0, (offset*4)+1) & 0xff) << 16;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= (intelflash_read(0, (offset*4)+2) & 0xff) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= (intelflash_read(0, (offset*4)+3) & 0xff) << 0;
	}
	return r;
}

static WRITE32_HANDLER(flashram_w)
{
	if (!(mem_mask & 0xff000000))
	{
		intelflash_write(0, (offset*4)+0, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		intelflash_write(0, (offset*4)+1, (data >> 16) & 0xff);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		intelflash_write(0, (offset*4)+2, (data >> 8) & 0xff);
	}
	if (!(mem_mask & 0x000000ff))
	{
		intelflash_write(0, (offset*4)+3, (data >> 0) & 0xff);
	}
}

static READ32_HANDLER(soundflash_r)
{
	UINT32 r = 0;
	int chip;
	if (offset >= 0 && offset < 0x200000/4)
	{
		chip = 1;
	}
	else
	{
		chip = 2;
	}

	offset &= 0x7ffff;

	if (!(mem_mask & 0xff000000))
	{
		r |= (intelflash_read(chip, (offset*4)+0) & 0xff) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= (intelflash_read(chip, (offset*4)+1) & 0xff) << 16;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= (intelflash_read(chip, (offset*4)+2) & 0xff) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= (intelflash_read(chip, (offset*4)+3) & 0xff) << 0;
	}
	return r;
}

static WRITE32_HANDLER(soundflash_w)
{
	int chip;
	if (offset >= 0 && offset < 0x200000/4)
	{
		chip = 1;
	}
	else
	{
		chip = 2;
	}

	offset &= 0x7ffff;

	if (!(mem_mask & 0xff000000))
	{
		intelflash_write(chip, (offset*4)+0, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		intelflash_write(chip, (offset*4)+1, (data >> 16) & 0xff);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		intelflash_write(chip, (offset*4)+2, (data >> 8) & 0xff);
	}
	if (!(mem_mask & 0x000000ff))
	{
		intelflash_write(chip, (offset*4)+3, (data >> 0) & 0xff);
	}
}

/*****************************************************************************/
/* ATAPI Interface */

#define BYTESWAP16(x)	((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00))

#if 1
#define ATAPI_ENDIAN(x)	(BYTESWAP16(x))
#else
#define ATAPI_ENDIAN(x)	(x)
#endif

#define ATAPI_CYCLES_PER_SECTOR (32000)	// plenty of time to allow DMA setup etc.  BIOS requires this be at least 2000, individual games may vary.

static UINT8 atapi_regs[16];
static SCSIInstance *atapi_device_data[2];

#define ATAPI_STAT_BSY	   0x80
#define ATAPI_STAT_DRDY    0x40
#define ATAPI_STAT_DMARDDF 0x20
#define ATAPI_STAT_SERVDSC 0x10
#define ATAPI_STAT_DRQ     0x08
#define ATAPI_STAT_CORR    0x04
#define ATAPI_STAT_CHECK   0x01

#define ATAPI_INTREASON_COMMAND 0x01
#define ATAPI_INTREASON_IO      0x02
#define ATAPI_INTREASON_RELEASE 0x04

#define ATAPI_REG_DATA		0
#define ATAPI_REG_ERRFEAT	1
#define ATAPI_REG_INTREASON	2
#define ATAPI_REG_SAMTAG	3
#define ATAPI_REG_COUNTLOW	4
#define ATAPI_REG_COUNTHIGH	5
#define ATAPI_REG_DRIVESEL	6
#define ATAPI_REG_CMDSTATUS	7

static UINT16 atapi_data[32*1024];
static UINT8  atapi_scsi_packet[32*1024];
static int atapi_data_ptr, atapi_xferlen, atapi_xfermod, atapi_cdata_wait;
static int atapi_drivesel;

static void atapi_cause_irq(void)
{
	cpunum_set_input_line(Machine, 0, INPUT_LINE_IRQ4, ASSERT_LINE);
}

static void atapi_exit(running_machine* machine)
{
	SCSIDeleteInstance(atapi_device_data[1]);
	SCSIDeleteInstance(atapi_device_data[0]);
}

static void atapi_init(running_machine *machine)
{
	memset(atapi_regs, 0, sizeof(atapi_regs));

	atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
	atapi_regs[ATAPI_REG_ERRFEAT] = 1;
	atapi_regs[ATAPI_REG_COUNTLOW] = 0x14;
	atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb;

	atapi_data_ptr = 0;
	atapi_cdata_wait = 0;

	// allocate two SCSI CD-ROM devices
	SCSIAllocInstance( SCSI_DEVICE_CDROM, &atapi_device_data[0], 0 );
	// TODO: the slave drive can be either CD-ROM, DVD-ROM or HDD
	SCSIAllocInstance( SCSI_DEVICE_CDROM, &atapi_device_data[1], 1 );
	add_exit_callback(machine, atapi_exit);
}

static void atapi_reset(void)
{
	logerror("ATAPI reset\n");

	atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
	atapi_regs[ATAPI_REG_ERRFEAT] = 1;
	atapi_regs[ATAPI_REG_COUNTLOW] = 0x14;
	atapi_regs[ATAPI_REG_COUNTHIGH] = 0xeb;

	atapi_data_ptr = 0;
	atapi_cdata_wait = 0;
}



static UINT16 atapi_command_reg_r(int reg)
{
	int i, data;
	static UINT8 temp_data[64*1024];

//  printf("ATAPI: Command reg read %d\n", reg);

	if (reg == ATAPI_REG_DATA)
	{
		// assert IRQ and drop DRQ
		if (atapi_data_ptr == 0)
		{
			//printf("ATAPI: dropping DRQ\n");
			atapi_cause_irq();
			atapi_regs[ATAPI_REG_CMDSTATUS] = 0;

			// get the data from the device
			SCSIReadData( atapi_device_data[atapi_drivesel], temp_data, atapi_xferlen );

			// fix it up in an endian-safe way
			for (i = 0; i < atapi_xferlen; i += 2)
			{
				atapi_data[i/2] = temp_data[i+0] | temp_data[i+1]<<8;
			}
		}

		data = atapi_data[atapi_data_ptr];
//      printf("ATAPI: %d, packet read = %04x\n", atapi_data_ptr, atapi_data[atapi_data_ptr]);
		atapi_data_ptr++;

		if (atapi_xfermod && atapi_data_ptr == (atapi_xferlen/2))
		{
			//printf("ATAPI: DRQ interrupt\n");
			atapi_cause_irq();
			atapi_regs[ATAPI_REG_CMDSTATUS] |= ATAPI_STAT_DRQ;
			atapi_data_ptr = 0;

			if (atapi_xfermod > 63488)
			{
				atapi_xfermod = atapi_xfermod - 63488;
				atapi_xferlen = 63488;
			}
			else
			{
				atapi_xferlen = atapi_xfermod;
				atapi_xfermod = 0;
			}

			//printf("ATAPI Transfer: %d, %d, %d\n", atapi_transfer_length, atapi_xfermod, atapi_xferlen);

			atapi_regs[ATAPI_REG_COUNTLOW] = atapi_xferlen & 0xff;
			atapi_regs[ATAPI_REG_COUNTHIGH] = (atapi_xferlen>>8)&0xff;
		}
		return data;
	}
	else
	{
		return atapi_regs[reg];
	}
}

static void atapi_command_reg_w(int reg, UINT16 data)
{
	int i;

	if (reg == ATAPI_REG_DATA)
	{
//      printf("ATAPI: packet write %04x at %08X\n", data, activecpu_get_pc());
		atapi_data[atapi_data_ptr] = data;
		atapi_data_ptr++;

		if (atapi_cdata_wait)
		{
//          printf("ATAPI: waiting, ptr %d wait %d\n", atapi_data_ptr, atapi_cdata_wait);
			if (atapi_data_ptr == atapi_cdata_wait)
			{
				// decompose SCSI packet into proper byte order
				for (i = 0; i < atapi_cdata_wait; i += 2)
				{
					atapi_scsi_packet[i] = atapi_data[i/2]&0xff;
					atapi_scsi_packet[i+1] = atapi_data[i/2]>>8;
				}

				// send it to the device
				SCSIWriteData( atapi_device_data[atapi_drivesel], atapi_scsi_packet, atapi_cdata_wait );

				// assert IRQ
				atapi_cause_irq();

				// not sure here, but clear DRQ at least?
				atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
			}
		}

		if ((!atapi_cdata_wait) && (atapi_data_ptr == 6))
		{
			int phase;

			// reset data pointer for reading SCSI results
			atapi_data_ptr = 0;

			atapi_regs[ATAPI_REG_CMDSTATUS] |= ATAPI_STAT_BSY;

			// assert IRQ
			atapi_cause_irq();

			// decompose SCSI packet into proper byte order
			for (i = 0; i < 16; i += 2)
			{
				atapi_scsi_packet[i+0] = atapi_data[i/2]&0xff;
				atapi_scsi_packet[i+1] = atapi_data[i/2]>>8;
			}

			// send it to the SCSI device
			SCSISetCommand( atapi_device_data[atapi_drivesel], atapi_scsi_packet, 12 );
			SCSIExecCommand( atapi_device_data[atapi_drivesel], &atapi_xferlen );
			SCSIGetPhase( atapi_device_data[atapi_drivesel], &phase );

			if (atapi_xferlen != -1)
			{
				logerror("ATAPI: SCSI command %02x returned %d bytes from the device\n", atapi_data[0]&0xff, atapi_xferlen);

				// store the returned command length in the ATAPI regs, splitting into
				// multiple transfers if necessary

				atapi_xfermod = 0;
				if (atapi_xferlen > 63488)
				{
					atapi_xfermod = atapi_xferlen - 63488;
					atapi_xferlen = 63488;
				}

//              printf("ATAPI Transfer: %d, %d\n", atapi_xfermod, atapi_xferlen);

				atapi_regs[ATAPI_REG_COUNTLOW] = atapi_xferlen & 0xff;
				atapi_regs[ATAPI_REG_COUNTHIGH] = (atapi_xferlen>>8)&0xff;

				switch( phase )
				{
				case SCSI_PHASE_DATAOUT:
					atapi_data_ptr = 0;
					atapi_cdata_wait = atapi_xferlen;
					logerror("ATAPI: Waiting for %x bytes of data\n", atapi_cdata_wait);
					break;
				}

				// perform special ATAPI processing of certain commands
				switch (atapi_data[0]&0xff)
				{
					case 0xa8:	// READ (12)
						// indicate data ready: set DRQ and DMA ready, and IO in INTREASON
						atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_SERVDSC;
						atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_IO;

						fatalerror("ATAPI: DMA read command attempted\n");
						break;

					case 0x00: // BUS RESET / TEST UNIT READY
					case 0xbb: // SET CD SPEED
					case 0xa5: // PLAY AUDIO
					case 0x1b:
					case 0x4e:
						atapi_regs[ATAPI_REG_CMDSTATUS] = 0;
						break;
				}
			}
			else
			{
//              printf("ATAPI: SCSI device returned error!\n");

				atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_DRQ | ATAPI_STAT_CHECK;
				atapi_regs[ATAPI_REG_ERRFEAT] = 0x50;	// sense key = ILLEGAL REQUEST
				atapi_regs[ATAPI_REG_COUNTLOW] = 0;
				atapi_regs[ATAPI_REG_COUNTHIGH] = 0;
			}
		}
	}
	else
	{
		data &= 0xff;
		atapi_regs[reg] = data;
//      printf("ATAPI: Command reg %d = %02X\n", reg, data);

		switch(reg)
		{
			case ATAPI_REG_DRIVESEL:
				atapi_drivesel = (data >> 4) & 0x1;
				break;

			case ATAPI_REG_CMDSTATUS:
//              printf("ATAPI: Issued command %02X\n", data);

				switch(data)
				{
					case 0x00:		/* NOP */
						break;

					case 0x08:		/* ATAPI Soft Reset */
						atapi_reset();
						break;

					case 0xa0:		/* ATAPI Packet */
			 			atapi_regs[ATAPI_REG_CMDSTATUS] = ATAPI_STAT_BSY | ATAPI_STAT_DRQ;
						atapi_regs[ATAPI_REG_INTREASON] = ATAPI_INTREASON_COMMAND;

						atapi_data_ptr = 0;
						atapi_cdata_wait = 0;
						break;

					default:
						fatalerror("ATAPI: Unknown ATA command %02X\n", data);
				}
				break;
		}
	}
}

static UINT16 atapi_control_reg_r(int reg)
{
	UINT16 value;
	switch(reg)
	{
		case 0x6:
		{
			value = atapi_regs[ATAPI_REG_CMDSTATUS];
			if (atapi_regs[ATAPI_REG_CMDSTATUS] & ATAPI_STAT_BSY)
			{
				atapi_regs[ATAPI_REG_CMDSTATUS] ^= ATAPI_STAT_BSY;
			}
			return value;
		}

		default:
			fatalerror("ATAPI: Read control reg %d\n", reg);
			break;
	}

	return 0;
}

static void atapi_control_reg_w(int reg, UINT16 data)
{
	switch(reg)
	{
		case 0x06:		/* Device Control */
		{
			if (data & 0x4)
			{
				atapi_reset();
			}
			break;
		}

		default:
			fatalerror("ATAPI: Control reg %d = %02X\n", reg, data);
	}
}



static READ32_HANDLER( atapi_command_r )
{
	UINT16 r;
//  printf("atapi_command_r: %08X, %08X\n", offset, mem_mask);
	if (mem_mask == 0x0000ffff)
	{
		r = atapi_command_reg_r(offset*2);
		return ATAPI_ENDIAN(r) << 16;
	}
	else
	{
		r = atapi_command_reg_r((offset*2) + 1);
		return ATAPI_ENDIAN(r) << 0;
	}
}

static WRITE32_HANDLER( atapi_command_w )
{
//  printf("atapi_command_w: %08X, %08X, %08X\n", data, offset, mem_mask);

	if (mem_mask == 0x0000ffff)
	{
		atapi_command_reg_w(offset*2, ATAPI_ENDIAN((data >> 16) & 0xffff));
	}
	else
	{
		atapi_command_reg_w((offset*2) + 1, ATAPI_ENDIAN((data >> 0) & 0xffff));
	}
}


static READ32_HANDLER( atapi_control_r )
{
	UINT16 r;
//  printf("atapi_control_r: %08X, %08X\n", offset, mem_mask);

	if (mem_mask == 0x0000ffff)
	{
		r = atapi_control_reg_r(offset*2);
		return ATAPI_ENDIAN(r) << 16;
	}
	else
	{
		r = atapi_control_reg_r((offset*2) + 1);
		return ATAPI_ENDIAN(r) << 0;
	}
}

static WRITE32_HANDLER( atapi_control_w )
{
	if (mem_mask == 0x0000ffff)
	{
		atapi_control_reg_w(offset*2, ATAPI_ENDIAN(data >> 16) & 0xff);
	}
	else
	{
		atapi_control_reg_w((offset*2) + 1, ATAPI_ENDIAN(data >> 0) & 0xff);
	}
}


/*****************************************************************************/

static READ32_HANDLER( comm_uart_r )
{
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= pc16552d_0_r((offset*4)+0) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= pc16552d_0_r((offset*4)+1) << 16;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= pc16552d_0_r((offset*4)+2) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= pc16552d_0_r((offset*4)+3) << 0;
	}

	return r;
}

static WRITE32_HANDLER( comm_uart_w )
{
	if (!(mem_mask & 0xff000000))
	{
		pc16552d_0_w((offset*4)+0, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		pc16552d_0_w((offset*4)+1, (data >> 16) & 0xff);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		pc16552d_0_w((offset*4)+2, (data >> 8) & 0xff);
	}
	if (!(mem_mask & 0x000000ff))
	{
		pc16552d_0_w((offset*4)+3, (data >> 0) & 0xff);
	}
}

static void comm_uart_irq_callback(int channel, int value)
{
	// TODO
	//cpunum_set_input_line(Machine, 0, INPUT_LINE_IRQ2, ASSERT_LINE);
}

/*****************************************************************************/
/* Epson RTC-65271 Real-time Clock/NVRAM */

static READ32_HANDLER( rtc_r )
{
	int reg = offset * 4;
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= rtc65271_r((reg >> 8) & 0x1, (reg & 0xff) + 0) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= rtc65271_r((reg >> 8) & 0x1, (reg & 0xff) + 1) << 16;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= rtc65271_r((reg >> 8) & 0x1, (reg & 0xff) + 2) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= rtc65271_r((reg >> 8) & 0x1, (reg & 0xff) + 3) << 0;
	}

	return r;
}

static WRITE32_HANDLER( rtc_w )
{
	int reg = offset * 4;
	if (!(mem_mask & 0xff000000))
	{
		rtc65271_w((reg >> 8) & 0x1, (reg & 0xff) + 0, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		rtc65271_w((reg >> 8) & 0x1, (reg & 0xff) + 1, (data >> 16) & 0xff);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		rtc65271_w((reg >> 8) & 0x1, (reg & 0xff) + 2, (data >> 8) & 0xff);
	}
	if (!(mem_mask & 0x000000ff))
	{
		rtc65271_w((reg >> 8) & 0x1, (reg & 0xff) + 3, (data >> 0) & 0xff);
	}
}

/*****************************************************************************/

static READ32_HANDLER( sound_r )
{
	UINT32 r = 0;

//  printf("sound_r: %08X, %08X\n", offset, mem_mask);

	if (!(mem_mask & 0xff000000))	/* External RAM read */
	{
		r |= YMZ280B_data_0_r(offset) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= YMZ280B_status_0_r(offset) << 16;
	}

	return r;
}

static WRITE32_HANDLER( sound_w )
{
//  printf("sound_w: %08X, %08X, %08X\n", offset, data, mem_mask);

	if (!(mem_mask & 0xff000000))
	{
		YMZ280B_register_0_w(offset, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		YMZ280B_data_0_w(offset, (data >> 16) & 0xff);
	}
}

static const int cab_data[2] = { 0x0, 0x8 };
static const int kbm_cab_data[2] = { 0x2, 0x8 };
static const int ppd_cab_data[2] = { 0x1, 0x9 };
static int cab_data_ptr = 0;
static const int * cur_cab_data = cab_data;

static READ32_HANDLER( cabinet_r )
{
	UINT32 r = 0;

//  printf("cabinet_r: %08X, %08X\n", offset, mem_mask);

	switch (offset)
	{
		case 0:
		{
			r = cur_cab_data[cab_data_ptr & 1] << 28;
			cab_data_ptr++;
			return r;
		}
		case 2:		return 0x00000000;
		case 4:		return 0x00000000;
	}

	return 0;
}

/*****************************************************************************/

static READ32_HANDLER( keyboard_wheel_r )
{
	if (offset == 0)		// Keyboard Wheel (P1)
	{
		return readinputport(3) << 24;
	}
	else if (offset == 2)	// Keyboard Wheel (P2)
	{
		return readinputport(4) << 24;
	}

	return 0;
}

static READ32_HANDLER( midi_uart_r )
{
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= pc16552d_1_r(offset >> 6) << 24;
	}

	return r;
}

static WRITE32_HANDLER( midi_uart_w )
{
	if (!(mem_mask & 0xff000000))
	{
		pc16552d_1_w(offset >> 6, (data >> 24) & 0xff);
	}
}

static void midi_uart_irq_callback(int channel, int value)
{
	if (channel == 0)
	{
		if ((extend_board_irq_enable & 0x02) == 0)
		{
			extend_board_irq_active |= 0x02;
			cpunum_set_input_line(Machine, 0, INPUT_LINE_IRQ1, ASSERT_LINE);
		}
	}
	else
	{
		if ((extend_board_irq_enable & 0x01) == 0)
		{
			extend_board_irq_active |= 0x01;
			cpunum_set_input_line(Machine, 0, INPUT_LINE_IRQ1, ASSERT_LINE);
		}
	}
}

static int keyboard_state[2] = { 0, 0 };

static const int keyboard_notes[24] =
{
	0x3c,	// C1
	0x3d,	// C1#
	0x3e,	// D1
	0x3f,	// D1#
	0x40,	// E1
	0x41,	// F1
	0x42,	// F1#
	0x43,	// G1
	0x44,	// G1#
	0x45,	// A1
	0x46,	// A1#
	0x47,	// B1
	0x48,	// C2
	0x49,	// C2#
	0x4a,	// D2
	0x4b,	// D2#
	0x4c,	// E2
	0x4d,	// F2
	0x4e,	// F2#
	0x4f,	// G2
	0x50,	// G2#
	0x51,	// A2
	0x52,	// A2#
	0x53,	// B2
};

static TIMER_CALLBACK( keyboard_timer_callback )
{
	static const int kb_uart_channel[2] = { 1, 0 };
	int keyboard;
	int i;

	for (keyboard=0; keyboard < 2; keyboard++)
	{
		UINT32 kbstate = readinputport(5 + keyboard);
		int uart_channel = kb_uart_channel[keyboard];

		if (kbstate != keyboard_state[keyboard])
		{
			for (i=0; i < 24; i++)
			{
				int kbnote = keyboard_notes[i];

				if ((keyboard_state[keyboard] & (1 << i)) != 0 && (kbstate & (1 << i)) == 0)
				{
					// key was on, now off -> send Note Off message
					pc16552d_rx_data(1, uart_channel, 0x80);
					pc16552d_rx_data(1, uart_channel, kbnote);
					pc16552d_rx_data(1, uart_channel, 0x7f);
				}
				else if ((keyboard_state[keyboard] & (1 << i)) == 0 && (kbstate & (1 << i)) != 0)
				{
					// key was off, now on -> send Note On message
					pc16552d_rx_data(1, uart_channel, 0x90);
					pc16552d_rx_data(1, uart_channel, kbnote);
					pc16552d_rx_data(1, uart_channel, 0x7f);
				}
			}
		}
		else
		{
			// no messages, send Active Sense message instead
			pc16552d_rx_data(1, uart_channel, 0xfe);
		}

		keyboard_state[keyboard] = kbstate;
	}
}

// Extend board IRQs
// 0x01: MIDI UART channel 2
// 0x02: MIDI UART channel 1
// 0x04: ?
// 0x08: ?
// 0x10: ?
// 0x20: ?

static READ32_HANDLER( extend_board_irq_r)
{
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= (~extend_board_irq_active) << 24;
	}

	return r;
}

static WRITE32_HANDLER( extend_board_irq_w )
{
//  printf("extend_board_irq_w: %08X, %08X, %08X\n", data, offset, mem_mask);

	if (!(mem_mask & 0xff000000))
	{
		extend_board_irq_active &= ~((data >> 24) & 0xff);

		extend_board_irq_enable = (data >> 24) & 0xff;
	}
}

/*****************************************************************************/

static WRITE32_HANDLER( lamp_output_w )
{
	// -------- -------- -------- xxxxxxxx   Status LEDs (active low)
	if (!(mem_mask & 0x000000ff))
	{
		output_set_value("status_led_0", (data & 0x01) ? 0 : 1);
		output_set_value("status_led_1", (data & 0x02) ? 0 : 1);
		output_set_value("status_led_2", (data & 0x04) ? 0 : 1);
		output_set_value("status_led_3", (data & 0x08) ? 0 : 1);
		output_set_value("status_led_4", (data & 0x10) ? 0 : 1);
		output_set_value("status_led_5", (data & 0x20) ? 0 : 1);
		output_set_value("status_led_6", (data & 0x40) ? 0 : 1);
		output_set_value("status_led_7", (data & 0x80) ? 0 : 1);
	}

//  printf("lamp_output_w: %08X, %08X, %08X\n", data, offset, mem_mask);
}

static WRITE32_HANDLER( lamp_output_kbm_w )
{
	lamp_output_w(offset, data, mem_mask);

	if (!(mem_mask & 0xff000000))
	{
		output_set_value("door_lamp",	(data & 0x10000000) ? 1 : 0);
		output_set_value("start1p",		(data & 0x01000000) ? 1 : 0);
		output_set_value("start2p",		(data & 0x02000000) ? 1 : 0);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		output_set_value("lamp1",		(data & 0x00000100) ? 1 : 0);
		output_set_value("lamp2",		(data & 0x00000200) ? 1 : 0);
		output_set_value("lamp3",		(data & 0x00000400) ? 1 : 0);
		output_set_value("neon",		(data & 0x00000800) ? 1 : 0);
	}
}

static WRITE32_HANDLER( lamp_output_ppp_w )
{
	lamp_output_w(offset, data, mem_mask);

	// ParaParaParadise lamps (active high)
	// 0x00000100 Left
	// 0x00000200 Right
	// 0x00000400 Door Lamp
	// 0x00000800 OK
	// 0x00008000 Slim
	// 0x01000000 Stage LED 0
	// 0x02000000 Stage LED 1
	// 0x04000000 Stage LED 2
	// 0x08000000 Stage LED 3
	// 0x00010000 Stage LED 4
	// 0x00020000 Stage LED 5
	// 0x00040000 Stage LED 6
	// 0x00080000 Stage LED 7
	if (!(mem_mask & 0x0000ff00))
	{
		output_set_value("left",			(data & 0x00000100) ? 1 : 0);
		output_set_value("right",			(data & 0x00000200) ? 1 : 0);
		output_set_value("door_lamp",		(data & 0x00000400) ? 1 : 0);
		output_set_value("ok",				(data & 0x00000800) ? 1 : 0);
		output_set_value("slim",			(data & 0x00008000) ? 1 : 0);
	}
	if (!(mem_mask & 0xff000000))
	{
		output_set_value("stage_led_0",		(data & 0x01000000) ? 1 : 0);
		output_set_value("stage_led_1",		(data & 0x02000000) ? 1 : 0);
		output_set_value("stage_led_2",		(data & 0x04000000) ? 1 : 0);
		output_set_value("stage_led_3",		(data & 0x08000000) ? 1 : 0);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		output_set_value("stage_led_4",		(data & 0x00010000) ? 1 : 0);
		output_set_value("stage_led_5",		(data & 0x00020000) ? 1 : 0);
		output_set_value("stage_led_6",		(data & 0x00040000) ? 1 : 0);
		output_set_value("stage_led_7",		(data & 0x00080000) ? 1 : 0);
	}
}

static WRITE32_HANDLER( lamp_output2_w )
{
//  printf("lamp_output2_w: %08X, %08X, %08X\n", data, offset, mem_mask);
}

static WRITE32_HANDLER( lamp_output2_ppp_w )
{
	lamp_output2_w(offset, data, mem_mask);

	// ParaParaParadise lamps (active high)
	// 0x00010000 Top LED 0
	// 0x00020000 Top LED 1
	// 0x00040000 Top LED 2
	// 0x00080000 Top LED 3
	// 0x00000001 Top LED 4
	// 0x00000002 Top LED 5
	// 0x00000004 Top LED 6
	// 0x00000008 Top LED 7
	if (!(mem_mask & 0x00ff0000))
	{
		output_set_value("top_led_0",		(data & 0x00010000) ? 1 : 0);
		output_set_value("top_led_1",		(data & 0x00020000) ? 1 : 0);
		output_set_value("top_led_2",		(data & 0x00040000) ? 1 : 0);
		output_set_value("top_led_3",		(data & 0x00080000) ? 1 : 0);
	}
	if (!(mem_mask & 0x000000ff))
	{
		output_set_value("top_led_4",		(data & 0x00000001) ? 1 : 0);
		output_set_value("top_led_5",		(data & 0x00000002) ? 1 : 0);
		output_set_value("top_led_6",		(data & 0x00000004) ? 1 : 0);
		output_set_value("top_led_7",		(data & 0x00000008) ? 1 : 0);
	}
}

static WRITE32_HANDLER( lamp_output3_w )
{
//  printf("lamp_output3_w: %08X, %08X, %08X\n", data, offset, mem_mask);
}

static WRITE32_HANDLER( lamp_output3_ppp_w )
{
	lamp_output3_w(offset, data, mem_mask);

	// ParaParaParadise lamps (active high)
	// 0x00010000 Lamp 0
	// 0x00040000 Lamp 1
	// 0x00100000 Lamp 2
	// 0x00400000 Lamp 3
	if (!(mem_mask & 0x00ff0000))
	{
		output_set_value("lamp_0",			(data & 0x00010000) ? 1 : 0);
		output_set_value("lamp_1",			(data & 0x00040000) ? 1 : 0);
		output_set_value("lamp_2",			(data & 0x00100000) ? 1 : 0);
		output_set_value("lamp_3",			(data & 0x00400000) ? 1 : 0);
	}
}

/*****************************************************************************/

static UINT8 spu_shared_ram[0x400];

static READ32_HANDLER(ppc_spu_share_r)
{
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= spu_shared_ram[(offset * 4) + 0] << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= spu_shared_ram[(offset * 4) + 1] << 16;
	}
	if (!(mem_mask & 0x0000ff0000))
	{
		r |= spu_shared_ram[(offset * 4) + 2] <<  8;
	}
	if (!(mem_mask & 0xff000000))
	{
		r |= spu_shared_ram[(offset * 4) + 3] <<  0;
	}

	return r;
}

static WRITE32_HANDLER(ppc_spu_share_w)
{
	if (!(mem_mask & 0xff000000))
	{
		spu_shared_ram[(offset * 4) + 0] = (data >> 24) & 0xff;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		spu_shared_ram[(offset * 4) + 1] = (data >> 16) & 0xff;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		spu_shared_ram[(offset * 4) + 2] = (data >>  8) & 0xff;
	}
	if (!(mem_mask & 0x000000ff))
	{
		spu_shared_ram[(offset * 4) + 3] = (data >>  0) & 0xff;
	}
}

#ifdef UNUSED_FUNCTION
static READ16_HANDLER(m68k_spu_share_r)
{
	return spu_shared_ram[offset] << 8;
}

static WRITE16_HANDLER(m68k_spu_share_w)
{
	spu_shared_ram[offset] = (data >> 8) & 0xff;
}
#endif

static READ16_HANDLER(spu_unk_r)
{
	return 0xffff;
}

/*****************************************************************************/

static ADDRESS_MAP_START( firebeat_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x70000000, 0x70000fff) AM_READWRITE(midi_uart_r, midi_uart_w)
	AM_RANGE(0x70006000, 0x70006003) AM_WRITE(extend_board_irq_w)
	AM_RANGE(0x70008000, 0x7000800f) AM_READ(keyboard_wheel_r)
	AM_RANGE(0x7000a000, 0x7000a003) AM_READ(extend_board_irq_r)
	AM_RANGE(0x74000000, 0x740003ff) AM_READWRITE(ppc_spu_share_r, ppc_spu_share_w) // SPU shared RAM
	AM_RANGE(0x7d000200, 0x7d00021f) AM_READ(cabinet_r)
	AM_RANGE(0x7d000340, 0x7d000347) AM_READ(sensor_r)
	AM_RANGE(0x7d000400, 0x7d000403) AM_READWRITE(sound_r, sound_w)
	AM_RANGE(0x7d000800, 0x7d000803) AM_READ(input_r)
	AM_RANGE(0x7d400000, 0x7d5fffff) AM_READWRITE(flashram_r, flashram_w)
	AM_RANGE(0x7d800000, 0x7dbfffff) AM_READWRITE(soundflash_r, soundflash_w)
	AM_RANGE(0x7dc00000, 0x7dc0000f) AM_READWRITE(comm_uart_r, comm_uart_w)
	AM_RANGE(0x7e000000, 0x7e00013f) AM_READWRITE(rtc_r, rtc_w)
	AM_RANGE(0x7e800000, 0x7e8000ff) AM_READWRITE(gcu0_r, gcu0_w)
	AM_RANGE(0x7e800100, 0x7e8001ff) AM_READWRITE(gcu1_r, gcu1_w)
	AM_RANGE(0x7fe00000, 0x7fe0000f) AM_READWRITE(atapi_command_r, atapi_command_w)
	AM_RANGE(0x7fe80000, 0x7fe8000f) AM_READWRITE(atapi_control_r, atapi_control_w)
	AM_RANGE(0x80000000, 0x81ffffff) AM_RAM
	AM_RANGE(0xfff80000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)		/* System BIOS */
ADDRESS_MAP_END

static ADDRESS_MAP_START( spu_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x13ffff) AM_RAM
	AM_RANGE(0x340000, 0x34000f) AM_READ(spu_unk_r)
ADDRESS_MAP_END

/*****************************************************************************/

static READ8_HANDLER( soundram_r )
{
	if (offset >= 0 && offset < 0x200000)
	{
		return intelflash_read(1, offset & 0x1fffff);
	}
	else if (offset >= 0x200000 && offset < 0x400000)
	{
		return intelflash_read(2, offset & 0x1fffff);
	}
	return 0;
}

static WRITE8_HANDLER( soundram_w )
{
}

static void sound_irq_callback(int state)
{
}

static const struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	sound_irq_callback,			// irq
	soundram_r,
	soundram_w,
};

static NVRAM_HANDLER(firebeat)
{
	nvram_handler_intelflash(machine, 0, file, read_or_write);
	nvram_handler_intelflash(machine, 1, file, read_or_write);
	nvram_handler_intelflash(machine, 2, file, read_or_write);

	if (read_or_write)
	{
		rtc65271_file_save(file);
	}
	else
	{
		rtc65271_init(xram, NULL);

		if (file != NULL)
		{
			rtc65271_file_load(file);
		}
	}
}

static INPUT_PORTS_START(ppp)
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )			// Left
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )			// Right
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW) 			// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service") PORT_CODE(KEYCODE_7)		// Service
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )				// Coin
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )				// Start / Ok
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Dip switches */

	// ParaParaParadise has 24 sensors, grouped into groups of 3 for each sensor bar
	// Sensors 15...23 are only used by the Korean version of PPP, which has 8 sensor bars

	PORT_START_TAG("SENSOR1")
	PORT_BIT( 0x00070000, IP_ACTIVE_HIGH, IPT_BUTTON3 )		// Sensor 0, 1, 2  (Sensor bar 1)
	PORT_BIT( 0x00380000, IP_ACTIVE_HIGH, IPT_BUTTON4 )		// Sensor 3, 4, 5  (Sensor bar 2)
	PORT_BIT( 0x00c00001, IP_ACTIVE_HIGH, IPT_BUTTON5 )		// Sensor 6, 7, 8  (Sensor bar 3)
	PORT_BIT( 0x0000000e, IP_ACTIVE_HIGH, IPT_BUTTON6 )		// Sensor 9, 10,11 (Sensor bar 4)

	PORT_START_TAG("SENSOR2")
	PORT_BIT( 0x00070000, IP_ACTIVE_HIGH, IPT_BUTTON7 )		// Sensor 12,13,14 (Sensor bar 5)
	PORT_BIT( 0x00380000, IP_ACTIVE_HIGH, IPT_BUTTON8 )		// Sensor 15,16,17 (Sensor bar 6)   (unused by PPP)
	PORT_BIT( 0x00c00001, IP_ACTIVE_HIGH, IPT_BUTTON9 )		// Sensor 18,19,20 (Sensor bar 7)   (unused by PPP)
	PORT_BIT( 0x0000000e, IP_ACTIVE_HIGH, IPT_BUTTON10 )	// Sensor 21,22,23 (Sensor bar 8)   (unused by PPP)

INPUT_PORTS_END

static INPUT_PORTS_START(kbm)
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )				// Start P1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )				// Start P2
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW) 			// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service") PORT_CODE(KEYCODE_7)		// Service
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )				// Coin
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )			// e-Amusement
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Dip switches */

	PORT_START_TAG("WHEEL_P1")			// Keyboard modulation wheel (P1)
	PORT_BIT( 0xff, 0x80, IPT_PADDLE_V ) PORT_MINMAX(0xff, 0x00) PORT_SENSITIVITY(30) PORT_KEYDELTA(10)

	PORT_START_TAG("WHEEL_P2")			// Keyboard modulation wheel (P2)
	PORT_BIT( 0xff, 0x80, IPT_PADDLE_V ) PORT_MINMAX(0xff, 0x00) PORT_SENSITIVITY(30) PORT_KEYDELTA(10)

	PORT_START_TAG("KEYBOARD_P1")
	PORT_BIT( 0x000001, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 C1") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x000002, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 C1#") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x000004, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 D1") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x000008, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 D1#") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x000010, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 E1") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x000020, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 F1") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x000040, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 F1#") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x000080, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 G1") PORT_CODE(KEYCODE_I)
	PORT_BIT( 0x000100, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 G1#") PORT_CODE(KEYCODE_O)
	PORT_BIT( 0x000200, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 A1") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x000400, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 A1#") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x000800, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 B1") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x001000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 C2") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x002000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 C2#") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x004000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 D2") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x008000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 D2#") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x010000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 E2") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x020000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 F2") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x040000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 F2#") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x080000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 G2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x100000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 G2#") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x200000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 A2") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x400000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 A2#") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x800000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 B2") PORT_CODE(KEYCODE_N)

	PORT_START_TAG("KEYBOARD_P2")
	PORT_BIT( 0x000001, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 C1") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x000002, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 C1#") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x000004, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 D1") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x000008, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 D1#") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x000010, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 E1") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x000020, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 F1") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x000040, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 F1#") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x000080, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 G1") PORT_CODE(KEYCODE_I)
	PORT_BIT( 0x000100, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 G1#") PORT_CODE(KEYCODE_O)
	PORT_BIT( 0x000200, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 A1") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x000400, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 A1#") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x000800, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 B1") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x001000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 C2") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x002000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 C2#") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x004000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 D2") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x008000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 D2#") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x010000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 E2") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0x020000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 F2") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x040000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 F2#") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x080000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 G2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x100000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 G2#") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x200000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 A2") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x400000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 A2#") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x800000, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 B2") PORT_CODE(KEYCODE_N)

INPUT_PORTS_END

static INPUT_PORTS_START(popn)
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )			// Switch 1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )			// Switch 2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )			// Switch 3
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )			// Switch 4
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 )			// Switch 5
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 )			// Switch 6
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7 )			// Switch 7
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON8 )			// Switch 8

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON9 )			// Switch 9
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )				// Coin
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE( 0x10, IP_ACTIVE_LOW)			// Test
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service") PORT_CODE(KEYCODE_7)		// Service
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Dip switches */

INPUT_PORTS_END

static INTERRUPT_GEN(firebeat_interrupt)
{
	// IRQs
	// IRQ 0: VBlank
	// IRQ 1: Extend board IRQ
	// IRQ 2: Main board UART
	// IRQ 4: ATAPI

	cpunum_set_input_line(machine, 0, INPUT_LINE_IRQ0, ASSERT_LINE);
}

static MACHINE_RESET( firebeat )
{
	void *cd;
	int i;
	UINT8 *sound = memory_region(REGION_SOUND1);

	for (i=0; i < 0x200000; i++)
	{
		sound[i] = intelflash_read(1, i);
		sound[i+0x200000] = intelflash_read(2, i);
	}

	SCSIGetDevice( atapi_device_data[1], &cd );
	cdda_set_cdrom(0, cd);
}

static const ppc_config firebeat_ppc_cfg =
{
	PPC_MODEL_403GCX
};

static MACHINE_DRIVER_START(firebeat)

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 66000000)
	MDRV_CPU_CONFIG(firebeat_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(firebeat_map, 0)
	MDRV_CPU_VBLANK_INT(firebeat_interrupt,1)

	MDRV_MACHINE_RESET(firebeat)
	MDRV_NVRAM_HANDLER(firebeat)

 	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(firebeat)
	MDRV_VIDEO_UPDATE(firebeat)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(CDDA, 0)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(firebeat2)

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 66000000)
	MDRV_CPU_CONFIG(firebeat_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(firebeat_map, 0)
	MDRV_CPU_VBLANK_INT(firebeat_interrupt,1)

	MDRV_MACHINE_RESET(firebeat)
	MDRV_NVRAM_HANDLER(firebeat)

 	/* video hardware */
	MDRV_PALETTE_LENGTH(32768)

	MDRV_SCREEN_ADD("left", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)

	MDRV_SCREEN_ADD("right", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)

	MDRV_VIDEO_START(firebeat)
	MDRV_VIDEO_UPDATE(firebeat)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(CDDA, 0)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(firebeat_spu)

	MDRV_IMPORT_FROM(firebeat)

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(spu_map, 0)

MACHINE_DRIVER_END

/*****************************************************************************/
/* Security dongle is a Dallas DS1411 RS232 Adapter with a DS1991 Multikey iButton */

typedef struct
{
	UINT8 identifier[8];
	UINT8 password[8];
	UINT8 data[0x30];
} IBUTTON_SUBKEY;

typedef struct
{
	IBUTTON_SUBKEY subkey[3];
} IBUTTON;

enum
{
	DS1991_STATE_NORMAL,
	DS1991_STATE_READ_SUBKEY,
};

static IBUTTON ibutton;

static int ibutton_state = DS1991_STATE_NORMAL;
static int ibutton_read_subkey_ptr = 0;

static UINT8 ibutton_subkey_data[0x40];

static void set_ibutton(UINT8 *data)
{
	int i, j;

	for (i=0; i < 3; i++)
	{
		// identifier
		for (j=0; j < 8; j++)
		{
			ibutton.subkey[i].identifier[j] = *data++;
		}

		// password
		for (j=0; j < 8; j++)
		{
			ibutton.subkey[i].password[j] = *data++;
		}

		// data
		for (j=0; j < 48; j++)
		{
			ibutton.subkey[i].data[j] = *data++;
		}
	}
}

static int ibutton_w(UINT8 data)
{
	int r = -1;

	switch (ibutton_state)
	{
		case DS1991_STATE_NORMAL:
		{
			switch (data)
			{
				//
				// DS2408B Serial 1-Wire Line Driver with Load Sensor
				//
				case 0xc1:			// DS2480B reset
				{
					r = 0xcd;
					break;
				}
				case 0xe1:			// DS2480B set data mode
				{
					break;
				}
				case 0xe3:			// DS2480B set command mode
				{
					break;
				}

				//
				// DS1991 MultiKey iButton
				//
				case 0x66:			// DS1991 Read SubKey
				{
					r = 0x66;
					ibutton_state = DS1991_STATE_READ_SUBKEY;
					ibutton_read_subkey_ptr = 0;
					break;
				}
				case 0xcc:			// DS1991 skip rom
				{
					r = 0xcc;
					ibutton_state = DS1991_STATE_NORMAL;
					break;
				}
				default:
				{
					fatalerror("ibutton: unknown normal mode cmd %02X\n", data);
					break;
				}
			}
			break;
		}

		case DS1991_STATE_READ_SUBKEY:
		{
			if (ibutton_read_subkey_ptr == 0)		// Read SubKey, 2nd command byte
			{
				int subkey = (data >> 6) & 0x3;
		//      printf("iButton SubKey %d\n", subkey);
				r = data;

				if (subkey < 3)
				{
					memcpy(&ibutton_subkey_data[0],  ibutton.subkey[subkey].identifier, 8);
					memcpy(&ibutton_subkey_data[8],  ibutton.subkey[subkey].password, 8);
					memcpy(&ibutton_subkey_data[16], ibutton.subkey[subkey].data, 0x30);
				}
				else
				{
					memset(&ibutton_subkey_data[0], 0, 0x40);
				}
			}
			else if (ibutton_read_subkey_ptr == 1)	// Read SubKey, 3rd command byte
			{
				r = data;
			}
			else
			{
				r = ibutton_subkey_data[ibutton_read_subkey_ptr-2];
			}
			ibutton_read_subkey_ptr++;
			if (ibutton_read_subkey_ptr >= 0x42)
			{
				ibutton_state = DS1991_STATE_NORMAL;
			}
			break;
		}
	}

	return r;
}

static void security_w(UINT8 data)
{
	int r = ibutton_w(data);
	if (r >= 0)
	{
		ppc403_spu_rx(r & 0xff);
	}
}

/*****************************************************************************/

static void init_lights(write32_handler out1, write32_handler out2, write32_handler out3)
{
	if(!out1) out1 = lamp_output_w;
	if(!out2) out1 = lamp_output2_w;
	if(!out3) out1 = lamp_output3_w;

	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x7d000804, 0x7d000807, 0, 0, out1);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x7d000320, 0x7d000323, 0, 0, out2);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x7d000324, 0x7d000327, 0, 0, out3);
}

static void init_firebeat(running_machine *machine)
{
	UINT8 *rom = memory_region(REGION_USER2);

	atapi_init(machine);
	intelflash_init(0, FLASH_FUJITSU_29F016A, NULL);
	intelflash_init(1, FLASH_FUJITSU_29F016A, NULL);
	intelflash_init(2, FLASH_FUJITSU_29F016A, NULL);

	rtc65271_init(xram, NULL);

	pc16552d_init(0, 19660800, comm_uart_irq_callback);		// Network UART
	pc16552d_init(1, 24000000, midi_uart_irq_callback);		// MIDI UART

	extend_board_irq_enable = 0x3f;
	extend_board_irq_active = 0x00;

	cur_cab_data = cab_data;

	ppc403_install_spu_tx_handler(security_w);

	set_ibutton(rom);

	init_lights(NULL, NULL, NULL);
}

static DRIVER_INIT(ppp)
{
	init_firebeat(machine);
	init_lights(lamp_output_ppp_w, lamp_output2_ppp_w, lamp_output3_ppp_w);
}

static DRIVER_INIT(ppd)
{
	init_firebeat(machine);
	init_lights(lamp_output_ppp_w, lamp_output2_ppp_w, lamp_output3_ppp_w);

	cur_cab_data = ppd_cab_data;
}

static void init_keyboard(void)
{
	// set keyboard timer
	keyboard_timer = timer_alloc(keyboard_timer_callback, NULL);
	timer_adjust_periodic(keyboard_timer, ATTOTIME_IN_MSEC(10), 0, ATTOTIME_IN_MSEC(10));
}

static DRIVER_INIT(kbm)
{
	init_firebeat(machine);
	init_lights(lamp_output_kbm_w, NULL, NULL);

	init_keyboard();

	cur_cab_data = kbm_cab_data;
}


/*****************************************************************************/

ROM_START( ppp )
	ROM_REGION32_BE(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("977jaa03.e21", 0x00000, 0x80000, CRC(7b83362a) SHA1(2857a93be58636c10a8d180dbccf2caeeaaff0e2))

	ROM_REGION(0x400000, REGION_SOUND1, ROMREGION_ERASE00)

	ROM_REGION(0xc0, REGION_USER2, 0)	// Security dongle
	ROM_LOAD("gq977-ja", 0x00, 0xc0, BAD_DUMP CRC(55b5abdb) SHA1(d8da5bac005235480a1815bd0a79c3e8a63ebad1))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "977jaa01", 0, MD5(9abc766b72dab28db920f3d264fc2254) SHA1(05bce40c3b241cd1f634d6688ec179a86f57da9f) )

	// TODO: the audio CD is not dumped
ROM_END

ROM_START( kbm )
	ROM_REGION32_BE(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("974a03.e21", 0x00000, 0x80000, CRC(ef9a932d) SHA1(6299d3b9823605e519dbf1f105b59a09197df72f))

	ROM_REGION(0x400000, REGION_SOUND1, ROMREGION_ERASE00)

	ROM_REGION(0xc0, REGION_USER2, ROMREGION_ERASE00)	// Security dongle
	ROM_LOAD("gq974-ja", 0x00, 0xc0, BAD_DUMP CRC(4578f29b) SHA1(faaeaf6357c1e86e898e7017566cfd2fc7ee3d6f))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "974jac01", 0, SHA1(18179bf23519d5b0c82c72e8f47dfaa4d2c4a3e2) MD5(cb68cf69e55aa33429f149f474e2c96e) )
	DISK_IMAGE_READONLY( "974jaa02", 1, SHA1(8d78a91d98967a232c4b98628e2db25df0a7f8bd) MD5(5cb0100791294559fedccc2a9a46fd86) )
ROM_END

ROM_START( kbm2nd )
	ROM_REGION32_BE(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("974a03.e21", 0x00000, 0x80000, CRC(ef9a932d) SHA1(6299d3b9823605e519dbf1f105b59a09197df72f))

	ROM_REGION(0x400000, REGION_SOUND1, ROMREGION_ERASE00)

	ROM_REGION(0xc0, REGION_USER2, ROMREGION_ERASE00)	// Security dongle
	ROM_LOAD("gca01-ja", 0x00, 0xc0, BAD_DUMP CRC(2bda339d) SHA1(031cb3f44e7a89cd62a9ba948f3d19d53a325abd))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "a01jaa01", 0, SHA1(87c21dc6b9fe8d9f696985cfd9dc14a23f0932fe) MD5(0eff2ca8ebef1fd8815d1d7cb0c2383a) )
	DISK_IMAGE_READONLY( "a01jaa02", 1, SHA1(fabfcc02f97c867c361df7b9539e6b77f369b73f) MD5(25679474e987d0dd83a0db2bad24bc14) )
ROM_END

ROM_START( kbm3rd )
	ROM_REGION32_BE(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("974a03.e21", 0x00000, 0x80000, CRC(ef9a932d) SHA1(6299d3b9823605e519dbf1f105b59a09197df72f))

	ROM_REGION(0x400000, REGION_SOUND1, ROMREGION_ERASE00)

	ROM_REGION(0xc0, REGION_USER2, 0)	// Security dongle
	ROM_LOAD("gca12-ja", 0x00, 0xc0, BAD_DUMP CRC(cf01dc15) SHA1(da8d208233487ebe65a0a9826fc72f1f459baa26))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "a12jaa01", 0, MD5(130a11bd229d9c30bd6d9ffeaf94926e) SHA1(6b595b17260b0ca17d04d7911616d5ff88158f26) )
	DISK_IMAGE_READONLY( "a12jaa02", 1, MD5(10ff654cf3d9b833ecbe72a395e7bb60) SHA1(4adddc8e028111169889bfb99007238da5f4d330) )
ROM_END

ROM_START( popn7 )
	ROM_REGION32_BE(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("a02jaa03.e21", 0x00000, 0x80000, CRC(43ecc093) SHA1(637df5b546cf7409dd4752dc471674fe2a046599))

	ROM_REGION(0x400000, REGION_SOUND1, ROMREGION_ERASE00)

	ROM_REGION(0xc0, REGION_USER2, ROMREGION_ERASE00)	// Security dongle
	ROM_LOAD("gcb00-ja", 0x00, 0xc0, BAD_DUMP CRC(cc28625a) SHA1(e7de79ae72fdbd22328c9de74dfa17b5e6ae43b6))

	ROM_REGION(0x80000, REGION_CPU2, 0)			// SPU 68K program
	ROM_LOAD16_WORD_SWAP("a02jaa04.3q", 0x00000, 0x80000, CRC(8c6000dd) SHA1(94ab2a66879839411eac6c673b25143d15836683))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "b00jab01", 0, SHA1(7462586f67b5c3b015ac581ad0afc089fcd6f537) MD5(af9a249b23783d53ff27ea7dc7e6735c) )
	DISK_IMAGE_READONLY( "b00jaa02", 1, SHA1(fea9439f14304d865830fb34f8781346d95a1df7) MD5(cf4c4f7c2321fcca6d86e8c144261752) )
ROM_END

ROM_START( ppd )
	ROM_REGION32_BE(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("977jaa03.e21", 0x00000, 0x80000, CRC(7b83362a) SHA1(2857a93be58636c10a8d180dbccf2caeeaaff0e2))

	ROM_REGION(0x400000, REGION_SOUND1, ROMREGION_ERASE00)

	ROM_REGION(0xc0, REGION_USER2, ROMREGION_ERASE00)	// Security dongle
	ROM_LOAD("gq977-ko", 0x00, 0xc0, BAD_DUMP CRC(ee743323) SHA1(2042e45879795557ad3cc21b37962f6bf54da60d))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "977kaa01", 0, SHA1(7069c1e42bf994ccdfcf6ff0dda9c5de94f1cc65) MD5(f499cb458d823200dc96fe9cef5c08c8) )
	DISK_IMAGE_READONLY( "977kaa02", 1, SHA1(45d5cda77f789351260bbd6f9c47a5fa93998133) MD5(b58978a81931058fe28825b6147b1bed) )
ROM_END

ROM_START( ppp11 )
	ROM_REGION32_BE(0x80000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("977jaa03.e21", 0x00000, 0x80000, CRC(7b83362a) SHA1(2857a93be58636c10a8d180dbccf2caeeaaff0e2))

	ROM_REGION(0x400000, REGION_SOUND1, ROMREGION_ERASE00)

	ROM_REGION(0xc0, REGION_USER2, ROMREGION_ERASE00)	// Security dongle
	ROM_LOAD("gq977-ja", 0x00, 0xc0, BAD_DUMP CRC(55b5abdb) SHA1(d8da5bac005235480a1815bd0a79c3e8a63ebad1))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "gc977jaa01", 0, SHA1(aa43526971dad6502e4b9583d8f5c18d93ced820) MD5(161ab0096d8def5ef133eec872afc645) )
	DISK_IMAGE_READONLY( "gc977jaa02", 1, SHA1(4a0edf424e091c33db91b00edf7c7246754cc8bb) MD5(9cb7cb79d2f0a47e994cfb91847ca190) )
ROM_END

/*****************************************************************************/

GAME( 2000, ppp,	  0,       firebeat,      ppp,    ppp,      ROT0,   "Konami",  "ParaParaParadise", GAME_NOT_WORKING);
GAME( 2000, ppd,      0,       firebeat,      ppp,    ppd,      ROT0,   "Konami",  "ParaParaDancing", GAME_NOT_WORKING);
GAME( 2000, ppp11,	  0,       firebeat,      ppp,    ppp,      ROT0,   "Konami",  "ParaParaParadise v1.1", GAME_NOT_WORKING);
GAMEL(2000, kbm,      0,       firebeat2,     kbm,    kbm,    ROT270,   "Konami",  "Keyboardmania", GAME_NOT_WORKING, layout_firebeat);
GAMEL(2000, kbm2nd,   0,       firebeat2,     kbm,    kbm,    ROT270,   "Konami",  "Keyboardmania 2nd Mix", GAME_NOT_WORKING, layout_firebeat);
GAMEL(2001, kbm3rd,   0,       firebeat2,     kbm,    kbm,    ROT270,   "Konami",  "Keyboardmania 3rd Mix", GAME_NOT_WORKING, layout_firebeat);
GAME( 2001, popn7,    0,       firebeat_spu,  popn,   ppp,      ROT0,   "Konami",  "Pop n' Music 7", GAME_NOT_WORKING);
