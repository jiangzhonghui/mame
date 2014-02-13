/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "emu.h"
#include "includes/wiz.h"


void wiz_state::video_start()
{
	save_item(NAME(m_char_bank));
	save_item(NAME(m_palbank));
	save_item(NAME(m_flipx));
	save_item(NAME(m_flipy));
	save_item(NAME(m_bgpen));
}

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Stinger has three 256x4 palette PROMs (one per gun).
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 100 ohm resistor  -- RED/GREEN/BLUE
        -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 1  kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void wiz_state::palette_init()
{
	const UINT8 *color_prom = memregion("proms")->base();
	int i;


	for (i = 0;i < machine().total_colors();i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;
		bit0 = (color_prom[machine().total_colors()] >> 0) & 0x01;
		bit1 = (color_prom[machine().total_colors()] >> 1) & 0x01;
		bit2 = (color_prom[machine().total_colors()] >> 2) & 0x01;
		bit3 = (color_prom[machine().total_colors()] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;
		bit0 = (color_prom[2*machine().total_colors()] >> 0) & 0x01;
		bit1 = (color_prom[2*machine().total_colors()] >> 1) & 0x01;
		bit2 = (color_prom[2*machine().total_colors()] >> 2) & 0x01;
		bit3 = (color_prom[2*machine().total_colors()] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;

		palette_set_color(machine(),i,MAKE_RGB(r,g,b));

		color_prom++;
	}
}

WRITE8_MEMBER(wiz_state::wiz_palettebank_w)
{
	m_palbank[offset] = data & 1;
	m_palette_bank = m_palbank[0] + 2 * m_palbank[1];
}

WRITE8_MEMBER(wiz_state::wiz_bgcolor_w)
{
	m_bgpen = data;
}

WRITE8_MEMBER(wiz_state::wiz_char_bank_select_w)
{
	m_char_bank[offset] = data & 1;
}

WRITE8_MEMBER(wiz_state::wiz_flipx_w)
{
	m_flipx = data;
}


WRITE8_MEMBER(wiz_state::wiz_flipy_w)
{
	m_flipy = data;
}

void wiz_state::draw_background(bitmap_ind16 &bitmap, const rectangle &cliprect, int bank, int colortype)
{
	UINT8 *videoram = m_videoram;
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

	for (offs = 0x400 - 1; offs >= 0; offs--)
	{
		int scroll,sx,sy,col;

		sx = offs % 32;
		sy = offs / 32;

		if (colortype)
		{
			col = (m_attributesram[2 * sx + 1] & 0x07);
		}
		else
		{
			col = (m_attributesram[2 * (offs % 32) + 1] & 0x04) + (videoram[offs] & 3);
		}

		scroll = (8*sy + 256 - m_attributesram[2 * sx]) % 256;
		if (m_flipy)
		{
			scroll = (248 - scroll) % 256;
		}
		if (m_flipx) sx = 31 - sx;


		machine().gfx[bank]->transpen(bitmap,cliprect,
			videoram[offs],
			col + 8 * m_palette_bank,
			m_flipx,m_flipy,
			8*sx,scroll,0);
	}
}

void wiz_state::draw_foreground(bitmap_ind16 &bitmap, const rectangle &cliprect, int colortype)
{
	int offs;

	/* draw the frontmost playfield. They are characters, but draw them as sprites. */
	for (offs = 0x400 - 1; offs >= 0; offs--)
	{
		int scroll,sx,sy,col;


		sx = offs % 32;
		sy = offs / 32;

		if (colortype)
		{
			col = (m_attributesram2[2 * sx + 1] & 0x07);
		}
		else
		{
			col = (m_colorram2[offs] & 0x07);
		}

		scroll = (8*sy + 256 - m_attributesram2[2 * sx]) % 256;
		if (m_flipy)
		{
			scroll = (248 - scroll) % 256;
		}
		if (m_flipx) sx = 31 - sx;


		machine().gfx[m_char_bank[1]]->transpen(bitmap,cliprect,
			m_videoram2[offs],
			col + 8 * m_palette_bank,
			m_flipx,m_flipy,
			8*sx,scroll,0);
	}
}

void wiz_state::draw_sprites(bitmap_ind16 &bitmap,const rectangle &cliprect, UINT8* sprite_ram,int bank)
{
	int offs;

	for (offs = m_spriteram.bytes() - 4;offs >= 0;offs -= 4)
	{
		int sx,sy;


		sx = sprite_ram[offs + 3];
		sy = sprite_ram[offs];

		if (!sx || !sy) continue;

		if ( m_flipx) sx = 240 - sx;
		if (!m_flipy) sy = 240 - sy;

		machine().gfx[bank]->transpen(bitmap,cliprect,
				sprite_ram[offs + 1],
				(sprite_ram[offs + 2] & 0x07) + 8 * m_palette_bank,
				m_flipx,m_flipy,
				sx,sy,0);
	}
}


UINT32 wiz_state::screen_update_kungfut(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_bgpen, cliprect);
	draw_background(bitmap, cliprect, 2 + m_char_bank[0] , 0);
	draw_foreground(bitmap, cliprect, 0);
	draw_sprites(bitmap, cliprect, m_spriteram2, 4);
	draw_sprites(bitmap, cliprect, m_spriteram  , 5);
	return 0;
}

UINT32 wiz_state::screen_update_wiz(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int bank;

	bitmap.fill(m_bgpen, cliprect);
	draw_background(bitmap, cliprect, 2 + ((m_char_bank[0] << 1) | m_char_bank[1]), 0);
	draw_foreground(bitmap, cliprect, 0);

	const rectangle spritevisiblearea(2*8, 32*8-1, 2*8, 30*8-1);
	const rectangle spritevisibleareaflipx(0*8, 30*8-1, 2*8, 30*8-1);
	const rectangle &visible_area = m_flipx ? spritevisibleareaflipx : spritevisiblearea;

	bank = 7 + *m_sprite_bank;

	draw_sprites(bitmap, visible_area, m_spriteram2, 6);
	draw_sprites(bitmap, visible_area, m_spriteram  , bank);
	return 0;
}


UINT32 wiz_state::screen_update_stinger(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_bgpen, cliprect);
	draw_background(bitmap, cliprect, 2 + m_char_bank[0], 1);
	draw_foreground(bitmap, cliprect, 1);
	draw_sprites(bitmap, cliprect, m_spriteram2, 4);
	draw_sprites(bitmap, cliprect, m_spriteram  , 5);
	return 0;
}
