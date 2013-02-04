/*

http://www.6502.org/users/andre/petindex/boards.html

Static Board (PET 2001)
-----------------------

Four variations based on type of RAM(6550 or 2114) and ROM(6540 or 2316B).
4K or 8K static RAM (selected by jumper).
40 column display
A video interrupt interferes with disk drive operation.
Display timing not compatible with Basic 4.0.
ROM sockets:  A2  2K character      ROM sockets:  A2  2K character
 (2316B)      H1  C000-CFFF           (6540)       H1  C000-C7FF
              H2  D000-DFFF                        H2  D000-D7FF
              H3  E000-E7FF                        H3  E000-E7FF
              H4  F000-FFFF                        H4  F000-F7FF
              H5  C000-CFFF                        H5  C800-CFFF
              H6  D000-DFFF                        H6  D800-DFFF
              H7  F000-FFFF                        H7  F800-FFFF


           IEEE user tape #2
     +------####-####--##-+
     !                    #
     !                    #
     !                    # exp
     !                    # bus
     !                    #
     !                    #    2000 Series
     !                    !       circa 1977/78  Max RAM - 8k
     !       (2k) ROMS    !       [w/daughter board exp to 32k shown]
     !      F F E D D C C !
     !      8 0 0 8 0 8 0 !
     !                    !
tape #       RAM MEMORY   !
 #1  #                    !
     +--------------------+


Dynamic Board (PET/CBM 2001-N/2001-B/4000)
------------------------------------------

4K, 8K, 16K or 32K dynamic RAM (selected by jumper).
40 column display
Can run all versions of 40 column Basic (Basic 1 must be copied to 4K ROMs)
Can be jumpered to replace the older board.
ROM sockets:  UD3   9000-9FFF
              UD4   A000-AFFF
              UD5   B000-BFFF
              UD6   C000-CFFF
              UD7   D000-DFFF
              UD8   E000-E7FF
              UD9   F000-FFFF
              UF10  2K character


            IEEE user tape #1
     +------####-####--##-+
     !                   #!
     !                   #!
     !                   #! exp
     !        ROMS       #! bus
     !    F E D C B A 9  #!
     !                   #!    3000, 4000 Series
     !                    !       (3000 series is European version)
     !                    !       circa 1979/80  Max RAM - 32k
     !                    !
     !                    !
     !                    !
tape #      RAM MEMORY    !
 #2  #                    !
     +--------------------+


80 Column Board (CBM 8000)
--------------------------

16K or 32K RAM (selected by jumper).
Uses CTRC to generate 80 column display.
Can only run the 80 column version of Basic 4.0.
Not compatible with older boards.
ROM sockets:  UA3   2K or 4K character
              UD6   F000-FFFF
              UD7   E000-E7FF
              UD8   D000-DFFF
              UD9   C000-CFFF
              UD10  B000-BFFF
              UD11  A000-AFFF
              UD12  9000-9FFF

The layout is the same of the one used in Universal Boards below.


Universal Board (CBM 8000/PET 4000-12)
--------------------------------------

This is an 80 column board with jumpers for different configurations.
16K or 32K RAM (selected by jumper).
Uses CTRC to generate 40 or 80 column display (selected by jumpers).
Can only run Basic 4.0 versions that support the CRTC.
Can be jumpered to replace all older boards.
ROM sockets:  UA3   2K or 4K character
              UD6   F000-FFFF
              UD7   E000-E7FF
              UD8   D000-DFFF
              UD9   C000-CFFF
              UD10  B000-BFFF
              UD11  A000-AFFF
              UD12  9000-9FFF


           IEEE user tape #1
     +------####-####--##-+
     !                  # # tape
     !                  # #  #2
     !  R       exp bus # !
     !  A                #!
     !  M             9  #!
     !                A  #!     4000, 8000 Series
     !  M          R  B   !        circa 1981     Max RAM - 32k*
     !  E          O  C   !       [8296 layout not shown]
     !  M          M  D   !
     !  O          S  E   !
     !  R             F   !
     !  Y                 !
     !                spkr!
     +--------------------+
*/

/*

	TODO:

	- accurate video timing for non-CRTC models
	- PET 4000-12 (40 column CRTC models)
	- SuperPET
	- 8096
		- 64k expansion
	- 8296
		- PLA dumps
		- high resolution graphics
		- rom software list

*/

#include "includes/pet2001.h"



//**************************************************************************
//  INTERRUPTS
//**************************************************************************

//-------------------------------------------------
//  check_interrupts -
//-------------------------------------------------

void pet_state::check_interrupts()
{
	int irq = m_via_irq || m_pia1a_irq || m_pia1b_irq || m_pia2a_irq || m_pia2b_irq || m_exp_irq;

	m_maincpu->set_input_line(M6502_IRQ_LINE, irq);
}


//-------------------------------------------------
//  update_speaker -
//-------------------------------------------------

void pet_state::update_speaker()
{
	if (m_speaker)
	{
		speaker_level_w(m_speaker, !(m_via_cb2 || m_pia1_pa7));
	}
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( pet_state::read )
{
	int sel = offset >> 12;
	int norom = m_exp->norom_r(space, offset, sel);
	UINT8 data = 0;

	switch (sel)
	{
	case SEL0: case SEL1: case SEL2: case SEL3:	case SEL4: case SEL5: case SEL6: case SEL7:
		if (offset < m_ram->size())
		{
			data = m_ram->pointer()[offset];
		}
		break;

	case SEL8:
		data = m_video_ram[offset & (m_video_ram_size - 1)];
		break;

	case SEL9: case SELA: case SELB: case SELC: case SELD: case SELF:
		if (norom)
		{
			data = m_rom->base()[offset - 0x9000];
		}
		break;

	case SELE:
		if (BIT(offset, 11))
		{
			if (BIT(offset, 4))
			{
				data = m_pia1->read(space, offset & 0x03);
			}
			if (BIT(offset, 5))
			{
				data = m_pia2->read(space, offset & 0x03);
			}
			if (BIT(offset, 6))
			{
				data = m_via->read(space, offset & 0x0f);
			}
			if (m_crtc && BIT(offset, 7) && BIT(offset, 0))
			{
				data = m_crtc->register_r(space, 0);
			}
		}
		else if (norom)
		{
			data = m_rom->base()[offset - 0x9000];
		}
		break;
	}

	return m_exp->read(space, offset, data, sel);
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( pet_state::write )
{
	int sel = offset >> 12;

	switch (sel)
	{
	case SEL0: case SEL1: case SEL2: case SEL3:	case SEL4: case SEL5: case SEL6: case SEL7:
		if (offset < m_ram->size())
		{
			m_ram->pointer()[offset] = data;
		}
		break;

	case SEL8:
		m_video_ram[offset & (m_video_ram_size - 1)] = data;
		break;

	case SELE:
		if (BIT(offset, 11))
		{
			if (BIT(offset, 4))
			{
				m_pia1->write(space, offset & 0x03, data);
			}
			if (BIT(offset, 5))
			{
				m_pia2->write(space, offset & 0x03, data);
			}
			if (BIT(offset, 6))
			{
				m_via->write(space, offset & 0x0f, data);
			}
			if (m_crtc && BIT(offset, 7))
			{
				if (BIT(offset, 0))
				{
					m_crtc->register_w(space, 0, data);
				}
				else
				{
					m_crtc->address_w(space, 0, data);
				}
			}
		}
		break;
	}

	m_exp->write(space, offset, data, sel);
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( pet2001_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( pet2001_mem, AS_PROGRAM, 8, pet_state )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(read, write)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( pet )
//-------------------------------------------------

static INPUT_PORTS_START( pet )
	PORT_START( "ROW0" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Crsr Right Left") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Home  Clr Screen") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_MINUS) PORT_CHAR(0x2190)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9)          PORT_CHAR('(')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7)          PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5)          PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3)          PORT_CHAR('#')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1)          PORT_CHAR('!')

	PORT_START( "ROW1" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Del  Inst") PORT_CODE(KEYCODE_DEL) PORT_CHAR(8) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Crsr Down Up") PORT_CODE(KEYCODE_PGUP) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0)          PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8)          PORT_CHAR('\\')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6)          PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4)          PORT_CHAR('$')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2)          PORT_CHAR('"')

	PORT_START( "ROW2" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9_PAD)      PORT_CHAR('9')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7_PAD)      PORT_CHAR('7')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91 Pi") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x2191) PORT_CHAR(0x03C0)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O)          PORT_CHAR('O')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U)          PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T)          PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E)          PORT_CHAR('E')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q)          PORT_CHAR('Q')

	PORT_START( "ROW3" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH_PAD)  PORT_CHAR('/')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8_PAD)      PORT_CHAR('8')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P)          PORT_CHAR('P')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I)          PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y)          PORT_CHAR('Y')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R)          PORT_CHAR('R')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W)          PORT_CHAR('W')

	PORT_START( "ROW4" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6_PAD)      PORT_CHAR('6')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4_PAD)      PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L)          PORT_CHAR('L')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J)          PORT_CHAR('J')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G)          PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D)          PORT_CHAR('D')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A)          PORT_CHAR('A')

	PORT_START( "ROW5" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ASTERISK)   PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5_PAD)      PORT_CHAR('5')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)      PORT_CHAR(':')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K)          PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H)          PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F)          PORT_CHAR('F')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S)          PORT_CHAR('S')

	PORT_START( "ROW6" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3_PAD)      PORT_CHAR('3')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1_PAD)      PORT_CHAR('1')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP)       PORT_CHAR(';')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M)          PORT_CHAR('M')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B)          PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C)          PORT_CHAR('C')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z)          PORT_CHAR('Z')

	PORT_START( "ROW7" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PLUS_PAD)   PORT_CHAR('+')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2_PAD)      PORT_CHAR('2')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH)      PORT_CHAR('?')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N)          PORT_CHAR('N')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V)          PORT_CHAR('V')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X)          PORT_CHAR('X')

	PORT_START( "ROW8" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS_PAD)  PORT_CHAR('-')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0_PAD)      PORT_CHAR('0')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Shift (Right)") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)  PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE)      PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Shift (Left)") PORT_CODE(KEYCODE_LSHIFT)

	PORT_START( "ROW9" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad =") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR('=')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL_PAD)    PORT_CHAR('.')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Stop Run") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('<')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('[')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Rvs Off") PORT_CODE(KEYCODE_TAB)

	PORT_START( "LOCK" )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( petb )
//-------------------------------------------------

INPUT_PORTS_START( petb )
	PORT_START( "ROW0" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Crsr Right Left") PORT_CODE(KEYCODE_PGUP) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8_PAD)      PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8)          PORT_CHAR('8') PORT_CHAR('\\')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5)          PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2)          PORT_CHAR('2') PORT_CHAR('"')

	PORT_START( "ROW1" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9_PAD)      PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(0x2191)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7_PAD)      PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0_PAD)      PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7)          PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4)          PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1)          PORT_CHAR('1') PORT_CHAR('!')

	PORT_START( "ROW2" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5_PAD)      PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)      PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K)          PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)  PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H)          PORT_CHAR('H')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F)          PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S)          PORT_CHAR('S')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC)        PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START( "ROW3" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6_PAD)      PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE)      PORT_CHAR('@')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L)          PORT_CHAR('L')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J)          PORT_CHAR('J')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G)          PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D)          PORT_CHAR('D')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A)          PORT_CHAR('A')

	PORT_START( "ROW4" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Del  Inst") PORT_CODE(KEYCODE_DEL) PORT_CHAR(8) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P)          PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I)          PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('\\')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y)          PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R)          PORT_CHAR('R')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W)          PORT_CHAR('W')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TAB)        PORT_CHAR('\t')

	PORT_START( "ROW5" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4_PAD)      PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('[')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O)          PORT_CHAR('O')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Crsr Down Up") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U)          PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T)          PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E)          PORT_CHAR('E')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q)          PORT_CHAR('Q')

	PORT_START( "ROW6" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3_PAD)      PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Shift (Right)") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL_PAD)    PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP)       PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B)          PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C)          PORT_CHAR('C')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Shift (Left)") PORT_CODE(KEYCODE_LSHIFT)

	PORT_START( "ROW7" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2_PAD)      PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Repeat") PORT_CODE(KEYCODE_LALT)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0)          PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N)          PORT_CHAR('N')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V)          PORT_CHAR('V')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z)          PORT_CHAR('Z')

	PORT_START( "ROW8" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1_PAD)      PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH)      PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Home  Clr Screen") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M)          PORT_CHAR('M')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X)          PORT_CHAR('X')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Rvs Off") PORT_CODE(KEYCODE_INSERT)

	PORT_START( "ROW9" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)      PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Stop Run") PORT_CODE(KEYCODE_END)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9)          PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6)          PORT_CHAR('6') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3)          PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(0x2190)

	PORT_START( "LOCK" )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( petb_de )
//-------------------------------------------------

INPUT_PORTS_START( petb_de )
	PORT_INCLUDE( petb )
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( petb_se )
//-------------------------------------------------

INPUT_PORTS_START( petb_se )
	PORT_INCLUDE( petb )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  via6522_interface via_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( pet_state::via_irq_w )
{
	m_via_irq = state;

	check_interrupts();
}

READ8_MEMBER( pet_state::via_pb_r )
{
	/*

	    bit     description

	    PB0     _NDAC IN
	    PB1     
	    PB2     
	    PB3     
	    PB4     
	    PB5     SYNC IN
	    PB6     _NRFD IN
	    PB7     _DAV IN

	*/

	UINT8 data = 0;

	// video sync
	data |= (m_crtc ? m_crtc->vsync_r() : m_sync) << 5;

	// IEEE-488
	data |= m_ieee->ndac_r();
	data |= m_ieee->nrfd_r() << 6;
	data |= m_ieee->dav_r() << 7;

	return data;
}

WRITE8_MEMBER( pet_state::via_pb_w )
{
	/*

	    bit     description

	    PB0     
	    PB1     _NRFD OUT
	    PB2     _ATN OUT
	    PB3     CASS WRITE
	    PB4     #2 CASS MOTOR
	    PB5     
	    PB6     
	    PB7     

	*/

	// IEEE-488
	m_ieee->nrfd_w(BIT(data, 1));
	m_ieee->atn_w(BIT(data, 2));

	// cassette
	m_cassette->write(BIT(data, 3));
	m_cassette2->write(BIT(data, 3));
	m_cassette2->motor_w(BIT(data, 4));
}

WRITE_LINE_MEMBER( pet_state::via_ca2_w )
{
	m_graphic = state;
}

WRITE_LINE_MEMBER( pet_state::via_cb2_w )
{
	m_via_cb2 = state;
	update_speaker();

	m_user->cb2_w(state);
}

const via6522_interface via_intf =
{
	DEVCB_DEVICE_MEMBER(PET_USER_PORT_TAG, pet_user_port_device, pa_r),
	DEVCB_DRIVER_MEMBER(pet_state, via_pb_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(PET_DATASSETTE_PORT2_TAG, pet_datassette_port_device, read),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(PET_USER_PORT_TAG, pet_user_port_device, pa_w),
	DEVCB_DRIVER_MEMBER(pet_state, via_pb_w),
	DEVCB_DEVICE_LINE_MEMBER(PET_USER_PORT_TAG, pet_user_port_device, ca1_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, via_ca2_w),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pet_state, via_cb2_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, via_irq_w)
};


//-------------------------------------------------
//  pia6821_interface pia1_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( pet_state::pia1_irqa_w )
{
	m_pia1a_irq = state;

	check_interrupts();
}

WRITE_LINE_MEMBER( pet_state::pia1_irqb_w )
{
	m_pia1b_irq = state;

	check_interrupts();
}

READ8_MEMBER( pet_state::pia1_pa_r )
{
	/*

	    bit     description

	    PA0     KEY A
	    PA1     KEY B
	    PA2     KEY C
	    PA3     KEY D
	    PA4     #1 CASS SWITCH
	    PA5     #2 CASS SWITCH
	    PA6     _EOI IN
	    PA7     DIAG JUMPER

	*/

	UINT8 data = 0;

	// keyboard
	data |= m_key;

	// cassette
	data |= m_cassette->sense_r() << 4;
	data |= m_cassette2->sense_r() << 5;

	// IEEE-488
	data |= m_ieee->eoi_r() << 6;

	// diagnostic jumper
	data |= 0x80;

	return data;
}

WRITE8_MEMBER( pet_state::pia1_pa_w )
{
	/*

	    bit     description

	    PA0     KEY A
	    PA1     KEY B
	    PA2     KEY C
	    PA3     KEY D
	    PA4     
	    PA5     
	    PA6     
	    PA7     SPEAKER

	*/

	// keyboard
	m_key = data & 0x0f;

	// speaker
	m_pia1_pa7 = BIT(data, 7);
	update_speaker();
}

READ8_MEMBER( pet_state::pia1_pb_r )
{
	UINT8 data = 0xff;

	switch (m_key)
	{
	case 0: data &= m_row0->read(); break;
	case 1: data &= m_row1->read(); break;
	case 2: data &= m_row2->read(); break;
	case 3: data &= m_row3->read(); break;
	case 4: data &= m_row4->read(); break;
	case 5: data &= m_row5->read(); break;
	case 6: data &= m_row6->read(); break;
	case 7: data &= m_row7->read(); break;
	case 8: data &= m_row8->read() & m_lock->read(); break;
	case 9: data &= m_row9->read(); break;
	}

	return data;
}

READ8_MEMBER( pet2001b_state::pia1_pb_r )
{
	UINT8 data = 0xff;

	switch (m_key)
	{
	case 0: data &= m_row0->read(); break;
	case 1: data &= m_row1->read(); break;
	case 2: data &= m_row2->read(); break;
	case 3: data &= m_row3->read(); break;
	case 4: data &= m_row4->read(); break;
	case 5: data &= m_row5->read(); break;
	case 6: data &= m_row6->read() & m_lock->read(); break;
	case 7: data &= m_row7->read(); break;
	case 8: data &= m_row8->read(); break;
	case 9: data &= m_row9->read(); break;
	}

	return data;
}

READ_LINE_MEMBER( pet_state::pia1_cb1_r )
{
	return (m_crtc ? m_crtc->vsync_r() : m_sync);
}

WRITE_LINE_MEMBER( pet_state::pia1_ca2_w )
{
	m_ieee->eoi_w(state);

	m_blanktv = state;
}

const pia6821_interface pia1_intf =
{
	DEVCB_DRIVER_MEMBER(pet_state, pia1_pa_r),
	DEVCB_DRIVER_MEMBER(pet_state, pia1_pb_r),
	DEVCB_DEVICE_LINE_MEMBER(PET_DATASSETTE_PORT_TAG, pet_datassette_port_device, read),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_cb1_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pet_state, pia1_pa_w),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_ca2_w),
	DEVCB_DEVICE_LINE_MEMBER(PET_DATASSETTE_PORT_TAG, pet_datassette_port_device, motor_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_irqa_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_irqb_w)
};

const pia6821_interface pet2001b_pia1_intf =
{
	DEVCB_DRIVER_MEMBER(pet_state, pia1_pa_r),
	DEVCB_DRIVER_MEMBER(pet2001b_state, pia1_pb_r),
	DEVCB_DEVICE_LINE_MEMBER(PET_DATASSETTE_PORT_TAG, pet_datassette_port_device, read),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_cb1_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pet_state, pia1_pa_w),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_ca2_w),
	DEVCB_DEVICE_LINE_MEMBER(PET_DATASSETTE_PORT_TAG, pet_datassette_port_device, motor_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_irqa_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia1_irqb_w)
};


//-------------------------------------------------
//  pia6821_interface pia2_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( pet_state::pia2_irqa_w )
{
	m_pia2a_irq = state;

	check_interrupts();
}

WRITE_LINE_MEMBER( pet_state::pia2_irqb_w )
{
	m_pia2b_irq = state;

	check_interrupts();
}

const pia6821_interface pia2_intf =
{
	DEVCB_DEVICE_MEMBER(IEEE488_TAG, ieee488_device, dio_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, atn_r),
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, srq_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(IEEE488_TAG, ieee488_device, dio_w),
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, ndac_w),
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, dav_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia2_irqa_w),
	DEVCB_DRIVER_LINE_MEMBER(pet_state, pia2_irqb_w)
};


//-------------------------------------------------
//  IEEE488_INTERFACE( ieee488_intf )
//-------------------------------------------------

static IEEE488_INTERFACE( ieee488_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(M6520_2_TAG, pia6821_device, cb1_w),
	DEVCB_DEVICE_LINE_MEMBER(M6520_2_TAG, pia6821_device, ca1_w),
	DEVCB_NULL
};


//-------------------------------------------------
//  PET_DATASSETTE_PORT_INTERFACE( datassette_intf )
//-------------------------------------------------

static PET_DATASSETTE_PORT_INTERFACE( datassette_intf )
{
	DEVCB_DEVICE_LINE_MEMBER(M6520_1_TAG, pia6821_device, ca1_w)
};


//-------------------------------------------------
//  PET_DATASSETTE_PORT_INTERFACE( datassette2_intf )
//-------------------------------------------------

static PET_DATASSETTE_PORT_INTERFACE( datassette2_intf )
{
	DEVCB_DEVICE_LINE_MEMBER(M6522_TAG, via6522_device, write_cb1)
};


//-------------------------------------------------
//  PET_USER_PORT_INTERFACE( user_intf )
//-------------------------------------------------

static PET_USER_PORT_INTERFACE( user_intf )
{
	DEVCB_DEVICE_LINE_MEMBER(M6522_TAG, via6522_device, write_ca1),
	DEVCB_DEVICE_LINE_MEMBER(M6522_TAG, via6522_device, write_cb2)
};



//**************************************************************************
//  VIDEO
//**************************************************************************

//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( sync_tick )
//-------------------------------------------------

TIMER_DEVICE_CALLBACK_MEMBER( pet_state::sync_tick )
{
	m_sync = !m_sync;

	m_pia1->cb1_w(m_sync);
}


//-------------------------------------------------
//  SCREEN_UPDATE( pet2001 )
//-------------------------------------------------

UINT32 pet_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	for (int y = 0; y < 200; y++)
	{
		for (int sx = 0; sx < 40; sx++)
		{
			int sy = y / 8;
			offs_t video_addr = (sy * 40) + sx;
			UINT8 lsd = m_video_ram[video_addr];

			int ra = y & 0x07;
			offs_t char_addr = (m_graphic << 10) | ((lsd & 0x7f) << 3) | ra;
			UINT8 data = m_char_rom->base()[char_addr];

			for (int x = 0; x < 8; x++, data <<= 1)
			{
				int color = (BIT(data, 7) ^ BIT(lsd, 7)) && m_blanktv;
				bitmap.pix32(y, (sx * 8) + x) = RGB_MONOCHROME_GREEN[color];
			}
		}
	}

	return 0;
}


//-------------------------------------------------
//  MC6845_INTERFACE( crtc_intf )
//-------------------------------------------------

static MC6845_UPDATE_ROW( pet80_update_row )
{
	pet80_state *state = device->machine().driver_data<pet80_state>();
	int x = 0;
	int char_rom_mask = state->m_char_rom->bytes() - 1;

	for (int column = 0; column < x_count; column++)
	{
		UINT8 lsd = 0, data = 0;
		UINT8 rra = ra & 0x07;
		int no_row = !(BIT(ra, 3) || BIT(ra, 4));
		int invert = BIT(ma, 12);
		int chr_option = BIT(ma, 13);

		// even character

		lsd = state->m_video_ram[((ma + column) << 1) & 0x7ff];

		offs_t char_addr = (chr_option << 11) | (state->m_graphic << 10) | ((lsd & 0x7f) << 3) | rra;
		data = state->m_char_rom->base()[char_addr & char_rom_mask];

		for (int bit = 0; bit < 8; bit++, data <<= 1)
		{
			int video = !((BIT(data, 7) ^ BIT(lsd, 7)) && no_row) ^ invert;
			bitmap.pix32(y, x++) = RGB_MONOCHROME_GREEN[video];
		}

		// odd character

		lsd = state->m_video_ram[(((ma + column) << 1) + 1) & 0x7ff];

		char_addr = (chr_option << 11) | (state->m_graphic << 10) | ((lsd & 0x7f) << 3) | rra;
		data = state->m_char_rom->base()[char_addr & char_rom_mask];

		for (int bit = 0; bit < 8; bit++, data <<= 1)
		{
			int video = !((BIT(data, 7) ^ BIT(lsd, 7)) && no_row) ^ invert;
			bitmap.pix32(y, x++) = RGB_MONOCHROME_GREEN[video];
		}
	}
}

static MC6845_INTERFACE( crtc_intf )
{
	SCREEN_TAG,
	false,
	2*8,
	NULL,
	pet80_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(M6520_1_TAG, pia6821_device, cb1_w),
	NULL
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( pet )
//-------------------------------------------------

MACHINE_START_MEMBER( pet_state, pet )
{
	// allocate memory
	m_video_ram.allocate(m_video_ram_size);

	// initialize memory
	UINT8 data = 0xff;

	for (offs_t offset = 0; offset < m_ram->size(); offset++)
	{
		m_ram->pointer()[offset] = data;
		if (!(offset % 64)) data ^= 0xff;
	}

	data = 0xff;

	for (offs_t offset = 0; offset < m_video_ram_size; offset++)
	{
		m_video_ram[offset] = data;
		if (!(offset % 64)) data ^= 0xff;
	}

	// state saving
	save_item(NAME(m_key));
	save_item(NAME(m_sync));
	save_item(NAME(m_graphic));
	save_item(NAME(m_blanktv));
	save_item(NAME(m_via_irq));
	save_item(NAME(m_pia1a_irq));
	save_item(NAME(m_pia1b_irq));
	save_item(NAME(m_pia2a_irq));
	save_item(NAME(m_pia2b_irq));
	save_item(NAME(m_exp_irq));
}


//-------------------------------------------------
//  MACHINE_START( pet2001 )
//-------------------------------------------------

MACHINE_START_MEMBER( pet_state, pet2001 )
{
	m_video_ram_size = 0x400;

	MACHINE_START_CALL_MEMBER(pet);
}


//-------------------------------------------------
//  MACHINE_RESET( pet )
//-------------------------------------------------

MACHINE_RESET_MEMBER( pet_state, pet )
{
	m_maincpu->reset();

	m_via->reset();
	m_pia1->reset();
	m_pia2->reset();

	m_exp->reset();
}


//-------------------------------------------------
//  MACHINE_START( pet80 )
//-------------------------------------------------

MACHINE_START_MEMBER( pet80_state, pet80 )
{
	m_video_ram_size = 0x800;

	MACHINE_START_CALL_MEMBER(pet);
}


//-------------------------------------------------
//  MACHINE_RESET( pet80 )
//-------------------------------------------------

MACHINE_RESET_MEMBER( pet80_state, pet80 )
{
	MACHINE_RESET_CALL_MEMBER(pet);

	m_crtc->reset();
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( 4k )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( 4k )
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4K")
	MCFG_RAM_EXTRA_OPTIONS("8K, 16K,32K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( 8k )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( 8k )
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("8K")
	MCFG_RAM_EXTRA_OPTIONS("16K,32K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( 8k )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( 16k )
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("16K")
	MCFG_RAM_EXTRA_OPTIONS("32K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( 8k )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( 32k )
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("32K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet )
//-------------------------------------------------

static MACHINE_CONFIG_START( pet, pet_state )
	MCFG_MACHINE_START_OVERRIDE(pet_state, pet2001)
	MCFG_MACHINE_RESET_OVERRIDE(pet_state, pet)

	// basic machine hardware
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_8MHz/8)
	MCFG_CPU_PROGRAM_MAP(pet2001_mem)

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(320, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MCFG_SCREEN_UPDATE_DRIVER(pet_state, screen_update)
	MCFG_TIMER_DRIVER_ADD_PERIODIC("sync_timer", pet_state, sync_tick, attotime::from_hz(120))

	// devices
	MCFG_VIA6522_ADD(M6522_TAG, XTAL_8MHz/8, via_intf)
	MCFG_PIA6821_ADD(M6520_1_TAG, pia1_intf)
	MCFG_PIA6821_ADD(M6520_2_TAG, pia2_intf)
	MCFG_CBM_IEEE488_ADD(ieee488_intf, "c4040")
	MCFG_PET_DATASSETTE_PORT_ADD(PET_DATASSETTE_PORT_TAG, datassette_intf, cbm_datassette_devices, "c2n", NULL)
	MCFG_PET_DATASSETTE_PORT_ADD(PET_DATASSETTE_PORT2_TAG, datassette2_intf, cbm_datassette_devices, NULL, NULL)
	MCFG_PET_EXPANSION_SLOT_ADD(PET_EXPANSION_SLOT_TAG, XTAL_8MHz/8, pet_expansion_cards, NULL, NULL)
	MCFG_PET_USER_PORT_ADD(PET_USER_PORT_TAG, user_intf, pet_user_port_cards, NULL, NULL)
	MCFG_QUICKLOAD_ADD("quickload", cbm_pet, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)

	// software lists
	MCFG_SOFTWARE_LIST_ADD("cass_list", "pet_cass")
	MCFG_SOFTWARE_LIST_ADD("flop_list", "pet_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001, pet )
	MCFG_FRAGMENT_ADD(4k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet20018 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet20018, pet )
	MCFG_FRAGMENT_ADD(8k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001n )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001n, pet )
	MCFG_CARTSLOT_ADD("9000")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_INTERFACE("pet_9000_rom")

	MCFG_CARTSLOT_ADD("a000")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_INTERFACE("pet_a000_rom")

	MCFG_CARTSLOT_ADD("b000")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_INTERFACE("pet_b000_rom")

	MCFG_SOFTWARE_LIST_ADD("rom_list", "pet_rom")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001n8 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001n8, pet2001n )
	MCFG_FRAGMENT_ADD(8k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001n16 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001n16, pet2001n )
	MCFG_FRAGMENT_ADD(16k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001n32 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001n32, pet2001n )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm3008 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm3008, pet2001n )
	MCFG_FRAGMENT_ADD(8k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm3016 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm3016, pet2001n )
	MCFG_FRAGMENT_ADD(16k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm3032 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm3032, pet2001n )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001b )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED_CLASS( pet2001b, pet2001n, pet2001b_state )
	MCFG_DEVICE_REMOVE(M6520_1_TAG)
	MCFG_PIA6821_ADD(M6520_1_TAG, pet2001b_pia1_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001b8 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001b8, pet2001b )
	MCFG_FRAGMENT_ADD(8k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001b16 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001b16, pet2001b )
	MCFG_FRAGMENT_ADD(16k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet2001b32 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet2001b32, pet2001b )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm3032b )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm3032b, pet2001b )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet4000 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet4000, pet2001n )
	MCFG_DEVICE_REMOVE("b000")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet4016 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet4016, pet4000 )
	// RAM not upgradeable
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("16K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet4032 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet4032, pet4000 )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm4000 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm4000, pet2001n )
	MCFG_DEVICE_REMOVE("b000")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm4016 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm4016, cbm4000 )
	// RAM not upgradeable
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("16K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm4032 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm4032, cbm4000 )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet4000b )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet4000b, pet2001b )
	MCFG_DEVICE_REMOVE("b000")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet4032b )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet4032b, pet4000b )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm4000b )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm4000b, pet2001b )
	MCFG_DEVICE_REMOVE("b000")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm4032b )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm4032b, cbm4000b )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet80 )
//-------------------------------------------------

static MACHINE_CONFIG_START( pet80, pet80_state )
	MCFG_MACHINE_START_OVERRIDE(pet80_state, pet80)
	MCFG_MACHINE_RESET_OVERRIDE(pet80_state, pet80)

	// basic machine hardware
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/16)
	MCFG_CPU_PROGRAM_MAP(pet2001_mem)

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(640, 250)
	MCFG_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 250 - 1)
	MCFG_SCREEN_UPDATE_DEVICE(MC6845_TAG, mc6845_device, screen_update)
	MCFG_MC6845_ADD(MC6845_TAG, MC6845, XTAL_16MHz/16, crtc_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_VIA6522_ADD(M6522_TAG, XTAL_16MHz/16, via_intf)
	MCFG_PIA6821_ADD(M6520_1_TAG, pia1_intf)
	MCFG_PIA6821_ADD(M6520_2_TAG, pia2_intf)
	MCFG_CBM_IEEE488_ADD(ieee488_intf, "c8050")
	MCFG_PET_DATASSETTE_PORT_ADD(PET_DATASSETTE_PORT_TAG, datassette_intf, cbm_datassette_devices, "c2n", NULL)
	MCFG_PET_DATASSETTE_PORT_ADD(PET_DATASSETTE_PORT2_TAG, datassette2_intf, cbm_datassette_devices, NULL, NULL)
	MCFG_PET_EXPANSION_SLOT_ADD(PET_EXPANSION_SLOT_TAG, XTAL_16MHz/16, pet_expansion_cards, NULL, NULL)
	MCFG_PET_USER_PORT_ADD(PET_USER_PORT_TAG, user_intf, pet_user_port_cards, NULL, NULL)
	MCFG_QUICKLOAD_ADD("quickload", cbm_pet, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)

	// software lists
	MCFG_SOFTWARE_LIST_ADD("cass_list", "pet_cass")
	MCFG_SOFTWARE_LIST_ADD("flop_list", "pet_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( pet8032 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( pet8032, pet80 )
	MCFG_FRAGMENT_ADD(32k)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( superpet )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED_CLASS( superpet, pet80, superpet_state )
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("96K")

	MCFG_SOFTWARE_LIST_ADD("flop_list2", "superpet_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm8096 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED_CLASS( cbm8096, pet80, cbm8096_state )
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("96K")

	//MCFG_SOFTWARE_LIST_ADD("flop_list2", "cbm8096_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm8296 )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED_CLASS( cbm8296, pet80, cbm8296_state )
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")

	MCFG_SOFTWARE_LIST_ADD("flop_list2", "cbm8296_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( cbm8296d )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( cbm8296d, cbm8296 )
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( pet2001 )
//-------------------------------------------------

ROM_START( pet2001 )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_DEFAULT_BIOS( "basic1r" )
	ROM_SYSTEM_BIOS( 0, "basic1o", "Original" )
	ROMX_LOAD( "901447-01.h1", 0x3000, 0x0800, CRC(a055e33a) SHA1(831db40324113ee996c434d38b4add3fd1f820bd), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "basic1r", "Revised" )
	ROMX_LOAD( "901447-09.h1", 0x3000, 0x0800, CRC(03cf16d0) SHA1(1330580c0614d3556a389da4649488ba04a60908), ROM_BIOS(2) )
	ROM_LOAD( "901447-02.h5", 0x3800, 0x0800, CRC(69fd8a8f) SHA1(70c0f4fa67a70995b168668c957c3fcf2c8641bd) )
	ROM_LOAD( "901447-03.h2", 0x4000, 0x0800, CRC(d349f2d4) SHA1(4bf2c20c51a63d213886957485ebef336bb803d0) )
	ROM_LOAD( "901447-04.h6", 0x4800, 0x0800, CRC(850544eb) SHA1(d293972d529023d8fd1f493149e4777b5c253a69) )
	ROM_LOAD( "901447-05.h3", 0x5000, 0x0800, CRC(9e1c5cea) SHA1(f02f5fb492ba93dbbd390f24c10f7a832dec432a) )
	ROM_LOAD( "901447-06.h4", 0x6000, 0x0800, CRC(661a814a) SHA1(960717282878e7de893d87242ddf9d1512be162e) )
	ROM_LOAD( "901447-07.h7", 0x6800, 0x0800, CRC(c4f47ad1) SHA1(d440f2510bc52e20c3d6bc8b9ded9cea7f462a9c) )

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-08.a2", 0x000, 0x800, CRC(54f32f45) SHA1(3e067cc621e4beafca2b90cb8f6dba975df2855b) )
ROM_END

#define rom_pet20018 rom_pet2001


//-------------------------------------------------
//  ROM( pet2001n )
//-------------------------------------------------

ROM_START( pet2001n )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "b000", 0x2000, 0x1000, ROM_MIRROR )
	ROM_LOAD( "901465-01.ud6", 0x3000, 0x1000, CRC(63a7fe4a) SHA1(3622111f486d0e137022523657394befa92bde44) )   // BASIC 2
	ROM_LOAD( "901465-02.ud7", 0x4000, 0x1000, CRC(ae4cb035) SHA1(1bc0ebf27c9bb62ad71bca40313e874234cab6ac) )   // BASIC 2
	ROM_LOAD( "901447-24.ud8", 0x5000, 0x0800, CRC(e459ab32) SHA1(5e5502ce32f5a7e387d65efe058916282041e54b) )   // Screen Editor (40 columns, no CRTC, Normal Keyb)
	ROM_LOAD( "901465-03.ud9", 0x6000, 0x1000, CRC(f02238e2) SHA1(38742bdf449f629bcba6276ef24d3daeb7da6e84) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.uf10", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )   // Character Generator
ROM_END

#define rom_pet2001n16 rom_pet2001n
#define rom_pet2001n32 rom_pet2001n
#define rom_cbm3008 rom_pet2001n
#define rom_cbm3016 rom_pet2001n
#define rom_cbm3032 rom_pet2001n


//-------------------------------------------------
//  ROM( pet2001b )
//-------------------------------------------------

ROM_START( pet2001b )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "b000", 0x2000, 0x1000, ROM_MIRROR )
	ROM_LOAD( "901465-01.ud6", 0x3000, 0x1000, CRC(63a7fe4a) SHA1(3622111f486d0e137022523657394befa92bde44) )   // BASIC 2
	ROM_LOAD( "901465-02.ud7", 0x4000, 0x1000, CRC(ae4cb035) SHA1(1bc0ebf27c9bb62ad71bca40313e874234cab6ac) )   // BASIC 2
	ROM_LOAD( "901474-01.ud8", 0x5000, 0x0800, CRC(05db957e) SHA1(174ace3a8c0348cd21d39cc864e2adc58b0101a9) )   // Screen Editor (40 columns, no CRTC, Business Keyb)
	ROM_LOAD( "901465-03.ud9", 0x6000, 0x1000, CRC(f02238e2) SHA1(38742bdf449f629bcba6276ef24d3daeb7da6e84) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.uf10", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )   // Character Generator
ROM_END

#define rom_pet2001b16 rom_pet2001b
#define rom_pet2001b32 rom_pet2001b
#define rom_cbm3032b rom_pet2001b


//-------------------------------------------------
//  ROM( pet4016 )
//-------------------------------------------------

ROM_START( pet4016 )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_DEFAULT_BIOS( "basic4r" )
	ROM_SYSTEM_BIOS( 0, "basic4", "Original" )
	ROMX_LOAD( "901465-19.ud5", 0x2000, 0x1000, CRC(3a5f5721) SHA1(bc2b7c99495fea3eda950ee9e3d6cabe448a452b), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "basic4r", "Revised" )
	ROMX_LOAD( "901465-23.ud5", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc), ROM_BIOS(2) ) // BASIC 4
	ROM_LOAD( "901465-20.ud6", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud7", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "901499-01.ud7", 0x5000, 0x0800, CRC(5f85bdf8) SHA1(8cbf086c1ce4dfb2a2fe24c47476dfb878493dee) )   // Screen Editor (40 columns, CRTC 60Hz, Normal Keyb?)
	ROM_LOAD( "901447-29.ud8", 0x5000, 0x0800, CRC(e5714d4c) SHA1(e88f56e5c54b0e8d8d4e8cb39a4647c803c1f51c) )   // Screen Editor (40 columns, no CRTC, Normal Keyb)
	ROM_LOAD( "901465-22.ud9", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.uf10", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )   // Character Generator
ROM_END

#define rom_pet4032 rom_pet4016


//-------------------------------------------------
//  ROM( cbm4016 )
//-------------------------------------------------

ROM_START( cbm4016 )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_DEFAULT_BIOS( "basic4r" )
	ROM_SYSTEM_BIOS( 0, "basic4", "Original" )
	ROMX_LOAD( "901465-19.ud5", 0x2000, 0x1000, CRC(3a5f5721) SHA1(bc2b7c99495fea3eda950ee9e3d6cabe448a452b), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "basic4r", "Revised" )
	ROMX_LOAD( "901465-23.ud5", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc), ROM_BIOS(2) ) // BASIC 4
	ROM_LOAD( "901465-20.ud6", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud7", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "901498-01.ud7", 0x5000, 0x0800, CRC(3370e359) SHA1(05af284c914d53a52987b5f602466de75765f650) )   // Screen Editor (40 columns, CRTC 50Hz, Normal Keyb?)
	ROM_LOAD( "901447-29.ud8", 0x5000, 0x0800, CRC(e5714d4c) SHA1(e88f56e5c54b0e8d8d4e8cb39a4647c803c1f51c) )   // Screen Editor (40 columns, no CRTC, Normal Keyb)
	ROM_LOAD( "901465-22.ud9", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.uf10", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )   // Character Generator
ROM_END

#define rom_cbm4032 rom_cbm4016


//-------------------------------------------------
//  ROM( pet4032b )
//-------------------------------------------------

ROM_START( pet4032b )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_DEFAULT_BIOS( "basic4r" )
	ROM_SYSTEM_BIOS( 0, "basic4", "Original" )
	ROMX_LOAD( "901465-19.ud5", 0x2000, 0x1000, CRC(3a5f5721) SHA1(bc2b7c99495fea3eda950ee9e3d6cabe448a452b), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "basic4r", "Revised" )
	ROMX_LOAD( "901465-23.ud5", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc), ROM_BIOS(2) ) // BASIC 4
	ROM_LOAD( "901465-20.ud6", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud7", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "901474-02.ud8", 0x5000, 0x0800, CRC(75ff4af7) SHA1(0ca5c4e8f532f914cb0bf86ea9900f20f0a655ce) )   // Screen Editor (40 columns, no CRTC, Business Keyb)
	ROM_LOAD( "901465-22.ud9", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.uf10", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )   // Character Generator
ROM_END

#define rom_cbm4032b rom_pet4032b


//-------------------------------------------------
//  ROM( pet8032 )
//-------------------------------------------------

ROM_START( pet8032 )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_LOAD( "901465-23.ud10", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc) )  // BASIC 4
	ROM_LOAD( "901465-20.ud9", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud8", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "901474-03.ud7", 0x5000, 0x0800, CRC(5674dd5e) SHA1(c605fa343fd77c73cbe1e0e9567e2f014f6e7e30) )   // Screen Editor (80 columns, CRTC 60Hz, Business Keyb)
	ROM_LOAD( "901465-22.ud6", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.ua3", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )    // Character Generator
ROM_END


//-------------------------------------------------
//  ROM( cbm8032 )
//-------------------------------------------------

ROM_START( cbm8032 )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_LOAD( "901465-23.ud10", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc) )  // BASIC 4
	ROM_LOAD( "901465-20.ud9", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud8", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "901474-04.ud7", 0x5000, 0x0800, CRC(abb000e7) SHA1(66887061b6c4ebef7d6efb90af9afd5e2c3b08ba) )   // Screen Editor (80 columns, CRTC 50Hz, Business Keyb)
	ROM_LOAD( "901465-22.ud6", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.ua3", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )    // Character Generator
ROM_END

#define rom_cbm8096 rom_cbm8032


//-------------------------------------------------
//  ROM( cbm8032_de )
//-------------------------------------------------

ROM_START( cbm8032_de )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_LOAD( "901465-23.ud10", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc) )  // BASIC 4
	ROM_LOAD( "901465-20.ud9", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud8", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "german.bin",    0x5000, 0x0800, CRC(1c1e597d) SHA1(7ac75ed73832847623c9f4f197fe7fb1a73bb41c) )
	ROM_LOAD( "901465-22.ud6", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "chargen.de", 0x0000, 0x800, CRC(3bb8cb87) SHA1(a4f0df13473d7f9cd31fd62cfcab11318e2fb1dc) )
ROM_END


//-------------------------------------------------
//  ROM( cbm8032_se )
//-------------------------------------------------

ROM_START( cbm8032_se )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_LOAD( "901465-23.ud10", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc) )  // BASIC 4
	ROM_LOAD( "901465-20.ud9", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud8", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "swedish.bin",   0x5000, 0x0800, CRC(75901dd7) SHA1(2ead0d83255a344a42bb786428353ca48d446d03) )   // It had a label "8000-UD7, SCREEN-04"
	ROM_LOAD( "901465-22.ud6", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-14.ua3", 0x0000, 0x800, CRC(48c77d29) SHA1(aa7c8ff844d16ec05e2b32acc586c58d9e35388c) )    // Character Generator
ROM_END


//-------------------------------------------------
//  ROM( superpet )
//-------------------------------------------------

ROM_START( superpet )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_CART_LOAD( "9000", 0x0000, 0x1000, ROM_MIRROR )
	ROM_CART_LOAD( "a000", 0x1000, 0x1000, ROM_MIRROR )
	ROM_LOAD( "901465-23.ud10", 0x2000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc) )  // BASIC 4
	ROM_LOAD( "901465-20.ud9", 0x3000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a) )   // BASIC 4
	ROM_LOAD( "901465-21.ud8", 0x4000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8) )   // BASIC 4
	ROM_LOAD( "901474-03.ud7", 0x5000, 0x0800, CRC(5674dd5e) SHA1(c605fa343fd77c73cbe1e0e9567e2f014f6e7e30) )   // Screen Editor (80 columns, CRTC 60Hz, Business Keyb)
	ROM_LOAD( "901465-22.ud6", 0x6000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806) )   // Kernal

	ROM_REGION( 0x7000, M6809_TAG, 0 )
	ROM_LOAD( "901898-01.u17", 0x1000, 0x1000, CRC(728a998b) SHA1(0414b3ab847c8977eb05c2fcc72efcf2f9d92871) )
	ROM_LOAD( "901898-02.u18", 0x2000, 0x1000, CRC(6beb7c62) SHA1(df154939b934d0aeeb376813ec1ba0d43c2a3378) )
	ROM_LOAD( "901898-03.u19", 0x3000, 0x1000, CRC(5db4983d) SHA1(6c5b0cce97068f8841112ba6d5cd8e568b562fa3) )
	ROM_LOAD( "901898-04.u20", 0x4000, 0x1000, CRC(f55fc559) SHA1(b42a2050a319a1ffca7868a8d8d635fadd37ec37) )
	ROM_LOAD( "901897-01.u21", 0x5000, 0x0800, CRC(b2cee903) SHA1(e8ce8347451a001214a5e71a13081b38b4be23bc) )
	ROM_LOAD( "901898-05.u22", 0x6000, 0x1000, CRC(f42df0cb) SHA1(9b4a5134d20345171e7303445f87c4e0b9addc96) )

	ROM_REGION( 0x800, "charom", 0 )
	ROM_LOAD( "901447-10.ua3", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )    // Character Generator
ROM_END

#define rom_mmf9000 rom_superpet


//-------------------------------------------------
//  ROM( cbm8296 )
//-------------------------------------------------

ROM_START( cbm8296 )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_LOAD( "324992-02.ue10", 0x0000, 0x1000, CRC(2bac5baf) SHA1(03aa866e4bc4e38e95983a6a82ba925e710bede8) ) // HiRes Emulator
	ROM_LOAD( "324993-02.ue9", 0x1000, 0x1000, CRC(57444531) SHA1(74aa39888a6bc95762de767fce883203daca0d34) ) // HiRes BASIC
	ROM_LOAD( "324746-01.ue7", 0x2000, 0x3000, CRC(7935b528) SHA1(5ab17ee70467152bf2130e3f48a2aa81e9df93c9) )   // BASIC 4
	ROM_CONTINUE(              0x6000, 0x1000 )
	ROM_LOAD( "8296.ue8", 0x5000, 0x800, CRC(a3475de6) SHA1(b715db83fd26458dfd254bef5c4aae636753f7f5) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "901447-10.uc5", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )    // Character Generator

	ROM_REGION( 0x20, "prom", 0 )
	ROM_LOAD( "74s288.uc2", 0x00, 0x20, CRC(06030665) SHA1(19dc91ca49ecc20e66c646ba480d2c3bc70a62e6) ) // video/RAM timing

	ROM_REGION( 0xf5, "pla1", 0 )
	ROM_LOAD( "324744-01.ue6", 0x00, 0xf5, NO_DUMP )

	ROM_REGION( 0xf5, "pla2", 0 )
	ROM_LOAD( "324745-01.ue5", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( cbm8296d )
//-------------------------------------------------

ROM_START( cbm8296d )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_LOAD( "324992-02.ue10", 0x0000, 0x1000, CRC(2bac5baf) SHA1(03aa866e4bc4e38e95983a6a82ba925e710bede8) ) // HiRes Emulator
	ROM_LOAD( "324993-02.ue9", 0x1000, 0x1000, CRC(57444531) SHA1(74aa39888a6bc95762de767fce883203daca0d34) ) // HiRes BASIC
	ROM_LOAD( "324746-01.ue7", 0x2000, 0x3000, CRC(7935b528) SHA1(5ab17ee70467152bf2130e3f48a2aa81e9df93c9) )   // BASIC 4
	ROM_CONTINUE(              0x6000, 0x1000 )
	ROM_LOAD( "324243-01.ue8", 0x5000, 0x1000, CRC(4000e833) SHA1(dafbdf8ba0a1fe7d7b9586ffbfc9e5390c0fcf6f) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "901447-10.uc5", 0x000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db) )    // Character Generator

	ROM_REGION( 0x20, "prom", 0 )
	ROM_LOAD( "74s288.uc2", 0x00, 0x20, CRC(06030665) SHA1(19dc91ca49ecc20e66c646ba480d2c3bc70a62e6) ) // video/RAM timing

	ROM_REGION( 0xf5, "pla1", 0 )
	ROM_LOAD( "324744-01.ue6", 0x00, 0xf5, NO_DUMP )

	ROM_REGION( 0xf5, "pla2", 0 )
	ROM_LOAD( "324745-01.ue5", 0x00, 0xf5, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( cbm8296d_de )
//-------------------------------------------------

ROM_START( cbm8296d_de )
	ROM_REGION( 0x7000, M6502_TAG, 0 )
	ROM_LOAD( "324992-02.ue10", 0x0000, 0x1000, CRC(2bac5baf) SHA1(03aa866e4bc4e38e95983a6a82ba925e710bede8) ) // HiRes Emulator
	ROM_LOAD( "324993-02.ue9", 0x1000, 0x1000, CRC(57444531) SHA1(74aa39888a6bc95762de767fce883203daca0d34) ) // HiRes BASIC
	ROM_LOAD( "324746-01.ue7", 0x2000, 0x3000, CRC(7935b528) SHA1(5ab17ee70467152bf2130e3f48a2aa81e9df93c9) )
	ROM_CONTINUE(              0x6000, 0x1000 )
	ROM_LOAD( "324243-04.ue8", 0x5000, 0x1000, CRC(3fe48897) SHA1(c218ff3168514f1d5e7822ae1b1ac3e161523b33) )

	ROM_REGION( 0x1000, "charom", 0 )
	ROM_LOAD( "324242-10.uc5", 0x0000, 0x1000, CRC(a5632a0f) SHA1(9616f7f18757cccefb702a945f954b644d5b17d1) )

	ROM_REGION( 0x20, "prom", 0 )
	ROM_LOAD( "74s288.uc2", 0x00, 0x20, CRC(06030665) SHA1(19dc91ca49ecc20e66c646ba480d2c3bc70a62e6) ) // video/RAM timing

	ROM_REGION( 0xf5, "pla1", 0 )
	ROM_LOAD( "324744-01.ue6", 0x00, 0xf5, NO_DUMP )

	ROM_REGION( 0xf5, "pla2", 0 )
	ROM_LOAD( "324745-01.ue5", 0x00, 0xf5, NO_DUMP )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME    	PARENT  	COMPAT  MACHINE     INPUT   	INIT                COMPANY                         FULLNAME        FLAGS
COMP( 1977,	pet2001,	0,			0,		pet2001,	pet,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-4",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1977,	pet20018,	pet2001,	0,		pet20018,	pet,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-8",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	pet2001n,	0,			0,		pet2001n8,	pet,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-N8",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	pet2001n16,	pet2001n,	0,		pet2001n16,	pet,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-N16",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	pet2001n32,	pet2001n,	0,		pet2001n32,	pet,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-N32",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	cbm3008,	pet2001n,	0,		cbm3008,	pet,		driver_device,	0,	"Commodore Business Machines",	"CBM 3008",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	cbm3016,	pet2001n,	0,		cbm3016,	pet,		driver_device,	0,	"Commodore Business Machines",	"CBM 3016",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	cbm3032,	pet2001n,	0,		cbm3032,	pet,		driver_device,	0,	"Commodore Business Machines",	"CBM 3032",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	pet2001b,	0,			0,		pet2001b8,	petb,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-B8",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	pet2001b16,	pet2001b,	0,		pet2001b16,	petb,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-B16",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	pet2001b32,	pet2001b,	0,		pet2001b32,	petb,		driver_device,	0,	"Commodore Business Machines",	"PET 2001-B32",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1979,	cbm3032b,	pet2001b,	0,		cbm3032b,	petb,		driver_device,	0,	"Commodore Business Machines",	"CBM 3032B",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1980,	pet4016,	0,			0,		pet4016,	pet,		driver_device,	0,	"Commodore Business Machines",	"PET 4016",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1980,	pet4032,	pet4016,	0,		pet4032,	pet,		driver_device,	0,	"Commodore Business Machines",	"PET 4032",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1980,	cbm4016,	pet4016,	0,		cbm4016,	pet,		driver_device,	0,	"Commodore Business Machines",	"CBM 4016",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1980,	cbm4032,	pet4016,	0,		cbm4032,	pet,		driver_device,	0,	"Commodore Business Machines",	"CBM 4032",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1980,	pet4032b,	0,			0,		pet4032b,	petb,		driver_device,	0,	"Commodore Business Machines",	"PET 4032B",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1980,	cbm4032b,	pet4032b,	0,		cbm4032b,	petb,		driver_device,	0,	"Commodore Business Machines",	"CBM 4032B",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND_HW )
COMP( 1980,	pet8032,	0,			0,		pet8032,	petb,		driver_device,	0,	"Commodore Business Machines",	"PET 8032",		GAME_SUPPORTS_SAVE )
COMP( 1981,	cbm8032,	pet8032,	0,		pet8032,	petb,		driver_device,	0,	"Commodore Business Machines",	"CBM 8032",		GAME_SUPPORTS_SAVE )
COMP( 1981,	cbm8032_de,	pet8032,	0,		pet8032,	petb_de,	driver_device,	0,	"Commodore Business Machines",	"CBM 8032 (Germany)",			GAME_SUPPORTS_SAVE )
COMP( 1981,	cbm8032_se,	pet8032,	0,		pet8032,	petb_se,	driver_device,	0,	"Commodore Business Machines",	"CBM 8032 (Sweden/Finland)",	GAME_SUPPORTS_SAVE )
COMP( 1981,	superpet,	pet8032,	0,		superpet,	petb,		driver_device,	0,	"Commodore Business Machines",	"SuperPET SP-9000",				GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1981,	mmf9000,	pet8032,	0,		superpet,	petb,		driver_device,	0,	"Commodore Business Machines",	"MicroMainFrame 9000",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1981,	cbm8096,	pet8032,	0,		cbm8096,	petb,		driver_device,	0,	"Commodore Business Machines",	"CBM 8096",						GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1984,	cbm8296,	0,			0,		cbm8296,	petb,		driver_device,	0,	"Commodore Business Machines",	"CBM 8296",						GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1984,	cbm8296d,	cbm8296,	0,		cbm8296d,	petb,		driver_device,	0,	"Commodore Business Machines",	"CBM 8296D",					GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1984,	cbm8296d_de,cbm8296,	0,		cbm8296d,	petb_de,	driver_device,	0,	"Commodore Business Machines",	"CBM 8296D (Germany)",			GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
