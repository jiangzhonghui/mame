/**************************************************************************
 *
 * 39in1.c - bootleg MAME-based "39-in-1" arcade PCB
 * Skeleton by R. Belmont, thanks to the Guru
 * PXA255 Peripheral hookup by MooglyGuy
 * Decrypt by Andreas Naive
 *
 * CPU: Intel Xscale PXA255 series @ 200 MHz, configured little-endian
 * Xscale PXA consists of:
 *    ARMv5TE instruction set without the FPU
 *    ARM standard MMU
 *    ARM DSP extensions
 *    VGA-ish frame buffer with some 2D acceleration features
 *    AC97 stereo audio CODEC
 *
 * PCB also contains a custom ASIC, probably used for the decryption
 *
 * TODO:
 *   PXA255 peripherals
 *
 **************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "cpu/arm7/arm7.h"
#include "cpu/arm7/arm7core.h"
#include "machine/eeprom.h"
#include "machine/pxa255.h"

static void pxa255_dma_irq_check(running_machine* machine);
static READ32_HANDLER( pxa255_dma_r );
static WRITE32_HANDLER( pxa255_dma_w );

static READ32_HANDLER( pxa255_i2s_r );
static WRITE32_HANDLER( pxa255_i2s_w );

static void pxa255_ostimer_irq_check(running_machine* machine);
static TIMER_CALLBACK( pxa255_ostimer_match );
static READ32_HANDLER( pxa255_ostimer_r );
static WRITE32_HANDLER( pxa255_ostimer_w );

static void pxa255_update_interrupts(running_machine* machine);
static void pxa255_set_irq_line(running_machine* machine, UINT32 line, int state);
static READ32_HANDLER( pxa255_intc_r );
static WRITE32_HANDLER( pxa255_intc_w );

static READ32_HANDLER( pxa255_gpio_r );
static WRITE32_HANDLER( pxa255_gpio_w );

static void pxa255_lcd_load_dma_descriptor(const address_space* space, UINT32 address, int channel);
static void pxa255_lcd_irq_check(running_machine* machine);
static void pxa255_lcd_dma_kickoff(running_machine* machine, int channel);
static void pxa255_lcd_check_load_next_branch(running_machine* machine, int channel);
static READ32_HANDLER( pxa255_lcd_r );
static WRITE32_HANDLER( pxa255_lcd_w );

static PXA255_DMA_Regs dma_regs;
static PXA255_I2S_Regs i2s_regs;
static PXA255_OSTMR_Regs ostimer_regs;
static PXA255_INTC_Regs intc_regs;
static PXA255_GPIO_Regs gpio_regs;
static PXA255_LCD_Regs lcd_regs;

#define VERBOSE_LEVEL ( 3 )

INLINE void ATTR_PRINTF(3,4) verboselog( running_machine* machine, int n_level, const char* s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[32768];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%s: %s", cpuexec_describe_context(machine), buf );
	}
}

/*

  PXA255 Inter-Integrated-Circuit Sound (I2S) Controller

  pg. 489 to 504, PXA255 Processor Developers Manual [278693-002].pdf

*/

static READ32_HANDLER( pxa255_i2s_r )
{
	switch(PXA255_I2S_BASE_ADDR | (offset << 2))
	{
		case PXA255_SACR0:
			verboselog( space->machine, 3, "pxa255_i2s_r: Serial Audio Controller Global Control Register: %08x & %08x\n", i2s_regs.sacr0, mem_mask );
			return i2s_regs.sacr0;
		case PXA255_SACR1:
			verboselog( space->machine, 3, "pxa255_i2s_r: Serial Audio Controller I2S/MSB-Justified Control Register: %08x & %08x\n", i2s_regs.sacr1, mem_mask );
			return i2s_regs.sacr1;
		case PXA255_SASR0:
			verboselog( space->machine, 3, "pxa255_i2s_r: Serial Audio Controller I2S/MSB-Justified Status Register: %08x & %08x\n", i2s_regs.sasr0, mem_mask );
			return i2s_regs.sasr0;
		case PXA255_SAIMR:
			verboselog( space->machine, 3, "pxa255_i2s_r: Serial Audio Interrupt Mask Register: %08x & %08x\n", i2s_regs.saimr, mem_mask );
			return i2s_regs.saimr;
		case PXA255_SAICR:
			verboselog( space->machine, 3, "pxa255_i2s_r: Serial Audio Interrupt Clear Register: %08x & %08x\n", i2s_regs.saicr, mem_mask );
			return i2s_regs.saicr;
		case PXA255_SADIV:
			verboselog( space->machine, 3, "pxa255_i2s_r: Serial Audio Clock Divider Register: %08x & %08x\n", i2s_regs.sadiv, mem_mask );
			return i2s_regs.sadiv;
		case PXA255_SADR:
			//verboselog( space->machine, 3, "pxa255_i2s_r: Serial Audio Data Register: %08x & %08x\n", i2s_regs.sadr, mem_mask );
			return i2s_regs.sadr;
		default:
			verboselog( space->machine, 0, "pxa255_i2s_r: Unknown address: %08x\n", PXA255_I2S_BASE_ADDR | (offset << 2));
			break;
	}
	return 0;
}


static WRITE32_HANDLER( pxa255_i2s_w )
{
	switch(PXA255_I2S_BASE_ADDR | (offset << 2))
	{
		case PXA255_SACR0:
			verboselog( space->machine, 3, "pxa255_i2s_w: Serial Audio Controller Global Control Register: %08x & %08x\n", data, mem_mask );
			i2s_regs.sacr0 = data & 0x0000ff3d;
			break;
		case PXA255_SACR1:
			verboselog( space->machine, 3, "pxa255_i2s_w: Serial Audio Controller I2S/MSB-Justified Control Register: %08x & %08x\n", data, mem_mask );
			i2s_regs.sacr1 = data & 0x00000039;
			break;
		case PXA255_SASR0:
			verboselog( space->machine, 3, "pxa255_i2s_w: Serial Audio Controller I2S/MSB-Justified Status Register: %08x & %08x\n", data, mem_mask );
			i2s_regs.sasr0 = data & 0x0000ff7f;
			break;
		case PXA255_SAIMR:
			verboselog( space->machine, 3, "pxa255_i2s_w: Serial Audio Interrupt Mask Register: %08x & %08x\n", data, mem_mask );
			i2s_regs.saimr = data & 0x00000078;
			break;
		case PXA255_SAICR:
			verboselog( space->machine, 3, "pxa255_i2s_w: Serial Audio Interrupt Clear Register: %08x & %08x\n", data, mem_mask );
			if(i2s_regs.saicr & PXA255_SAICR_ROR)
			{
				i2s_regs.sasr0 &= ~PXA255_SASR0_ROR;
			}
			if(i2s_regs.saicr & PXA255_SAICR_TUR)
			{
				i2s_regs.sasr0 &= ~PXA255_SASR0_TUR;
			}
			break;
		case PXA255_SADIV:
			verboselog( space->machine, 3, "pxa255_i2s_w: Serial Audio Clock Divider Register: %08x & %08x\n", data, mem_mask );
			i2s_regs.sadiv = data & 0x0000007f;
			break;
		case PXA255_SADR:
			verboselog( space->machine, 4, "pxa255_i2s_w: Serial Audio Data Register: %08x & %08x\n", data, mem_mask );
			i2s_regs.sadr = data;
			break;
		default:
			verboselog( space->machine, 0, "pxa255_i2s_w: Unknown address: %08x = %08x & %08x\n", PXA255_I2S_BASE_ADDR | (offset << 2), data, mem_mask);
			break;
	}
}

/*

  PXA255 DMA controller (placeholder)

  pg. 151 to 182, PXA255 Processor Developers Manual [278693-002].pdf

*/

static void pxa255_dma_irq_check(running_machine* machine)
{
	int channel = 0;
	int set_intr = 0;
	for(channel = 0; channel < 16; channel++)
	{
		if(dma_regs.dcsr[channel] & (PXA255_DCSR_ENDINTR | PXA255_DCSR_STARTINTR | PXA255_DCSR_BUSERRINTR))
		{
			dma_regs.dint |= 1 << channel;
			set_intr = 1;
		}
		else
		{
			dma_regs.dint &= ~(1 << channel);
		}
	}
	pxa255_set_irq_line(machine, PXA255_INT_DMA, set_intr);
}

static TIMER_CALLBACK( pxa255_dma_dma_end )
{
	UINT32 sadr = dma_regs.dsadr[param];
	UINT32 tadr = dma_regs.dtadr[param];
	UINT32 count = dma_regs.dcmd[param] & 0x00001fff;
	UINT32 index = 0;
	dma_regs.dcsr[param] &= ~PXA255_DCSR_RUN;
	dma_regs.dcsr[param] |= PXA255_DCSR_STOPSTATE;
	for(index = 0; index < count; index++)
	{
		switch(dma_regs.dcmd[param] & PXA255_DCMD_SIZE)
		{
			case PXA255_DCMD_SIZE_8:
				memory_write_byte_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), tadr,
										memory_read_byte_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), sadr));
				break;
			case PXA255_DCMD_SIZE_16:
				memory_write_word_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), tadr,
									   memory_read_word_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), sadr));
				break;
			case PXA255_DCMD_SIZE_32:
				memory_write_dword_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), tadr,
										memory_read_dword_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), sadr));
				break;
			default:
				printf( "pxa255_dma_dma_end: Unsupported DMA size\n" );
				break;
		}
		if(dma_regs.dcmd[param] & PXA255_DCMD_INCSRCADDR)
		{
			switch(dma_regs.dcmd[param] & PXA255_DCMD_SIZE)
			{
				case PXA255_DCMD_SIZE_8:
					sadr++;
					break;
				case PXA255_DCMD_SIZE_16:
					sadr += 2;
					break;
				case PXA255_DCMD_SIZE_32:
					sadr += 4;
					break;
				default:
					break;
			}
		}
		if(dma_regs.dcmd[param] & PXA255_DCMD_INCTRGADDR)
		{
			switch(dma_regs.dcmd[param] & PXA255_DCMD_SIZE)
			{
				case PXA255_DCMD_SIZE_8:
					tadr++;
					break;
				case PXA255_DCMD_SIZE_16:
					tadr += 2;
					break;
				case PXA255_DCMD_SIZE_32:
					tadr += 4;
					break;
				default:
					break;
			}
		}
	}
	if(dma_regs.dcmd[param] & PXA255_DCMD_ENDIRQEN)
	{
		dma_regs.dcsr[param] |= PXA255_DCSR_ENDINTR;
	}
	pxa255_dma_irq_check(machine);
}

static READ32_HANDLER( pxa255_dma_r )
{
	switch(PXA255_DMA_BASE_ADDR | (offset << 2))
	{
		case PXA255_DCSR0:		case PXA255_DCSR1:		case PXA255_DCSR2:		case PXA255_DCSR3:
		case PXA255_DCSR4:		case PXA255_DCSR5:		case PXA255_DCSR6:		case PXA255_DCSR7:
		case PXA255_DCSR8:		case PXA255_DCSR9:		case PXA255_DCSR10:		case PXA255_DCSR11:
		case PXA255_DCSR12:		case PXA255_DCSR13:		case PXA255_DCSR14:		case PXA255_DCSR15:
			verboselog( space->machine, 0, "pxa255_dma_r: DMA Channel Control/Status Register %d: %08x & %08x\n", offset, dma_regs.dcsr[offset], mem_mask );
			return dma_regs.dcsr[offset];
		case PXA255_DINT:
			verboselog( space->machine, 0, "pxa255_dma_r: DMA Interrupt Register: %08x & %08x\n", dma_regs.dint, mem_mask );
			return dma_regs.dint;
		case PXA255_DRCMR0:		case PXA255_DRCMR1:		case PXA255_DRCMR2:		case PXA255_DRCMR3:
		case PXA255_DRCMR4:		case PXA255_DRCMR5:		case PXA255_DRCMR6:		case PXA255_DRCMR7:
		case PXA255_DRCMR8:		case PXA255_DRCMR9:		case PXA255_DRCMR10:	case PXA255_DRCMR11:
		case PXA255_DRCMR12:	case PXA255_DRCMR13:	case PXA255_DRCMR14:	case PXA255_DRCMR15:
		case PXA255_DRCMR16:	case PXA255_DRCMR17:	case PXA255_DRCMR18:	case PXA255_DRCMR19:
		case PXA255_DRCMR20:	case PXA255_DRCMR21:	case PXA255_DRCMR22:	case PXA255_DRCMR23:
		case PXA255_DRCMR24:	case PXA255_DRCMR25:	case PXA255_DRCMR26:	case PXA255_DRCMR27:
		case PXA255_DRCMR28:	case PXA255_DRCMR29:	case PXA255_DRCMR30:	case PXA255_DRCMR31:
		case PXA255_DRCMR32:	case PXA255_DRCMR33:	case PXA255_DRCMR34:	case PXA255_DRCMR35:
		case PXA255_DRCMR36:	case PXA255_DRCMR37:	case PXA255_DRCMR38:	case PXA255_DRCMR39:
			verboselog( space->machine, 0, "pxa255_dma_r: DMA Request to Channel Map Register %d: %08x & %08x\n", offset - (0x100 >> 2), 0, mem_mask );
			return dma_regs.drcmr[offset - (0x100 >> 2)];
		case PXA255_DDADR0:		case PXA255_DDADR1:		case PXA255_DDADR2:		case PXA255_DDADR3:
		case PXA255_DDADR4:		case PXA255_DDADR5:		case PXA255_DDADR6:		case PXA255_DDADR7:
		case PXA255_DDADR8:		case PXA255_DDADR9:		case PXA255_DDADR10:	case PXA255_DDADR11:
		case PXA255_DDADR12:	case PXA255_DDADR13:	case PXA255_DDADR14:	case PXA255_DDADR15:
			verboselog( space->machine, 0, "pxa255_dma_r: DMA Descriptor Address Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, 0, mem_mask );
			return dma_regs.ddadr[(offset - (0x200 >> 2)) >> 2];
		case PXA255_DSADR0:		case PXA255_DSADR1:		case PXA255_DSADR2:		case PXA255_DSADR3:
		case PXA255_DSADR4:		case PXA255_DSADR5:		case PXA255_DSADR6:		case PXA255_DSADR7:
		case PXA255_DSADR8:		case PXA255_DSADR9:		case PXA255_DSADR10:	case PXA255_DSADR11:
		case PXA255_DSADR12:	case PXA255_DSADR13:	case PXA255_DSADR14:	case PXA255_DSADR15:
			verboselog( space->machine, 0, "pxa255_dma_r: DMA Source Address Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, 0, mem_mask );
			return dma_regs.dsadr[(offset - (0x200 >> 2)) >> 2];
		case PXA255_DTADR0:		case PXA255_DTADR1:		case PXA255_DTADR2:		case PXA255_DTADR3:
		case PXA255_DTADR4:		case PXA255_DTADR5:		case PXA255_DTADR6:		case PXA255_DTADR7:
		case PXA255_DTADR8:		case PXA255_DTADR9:		case PXA255_DTADR10:	case PXA255_DTADR11:
		case PXA255_DTADR12:	case PXA255_DTADR13:	case PXA255_DTADR14:	case PXA255_DTADR15:
			verboselog( space->machine, 0, "pxa255_dma_r: DMA Target Address Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, 0, mem_mask );
			return dma_regs.dtadr[(offset - (0x200 >> 2)) >> 2];
		case PXA255_DCMD0:		case PXA255_DCMD1:		case PXA255_DCMD2:		case PXA255_DCMD3:
		case PXA255_DCMD4:		case PXA255_DCMD5:		case PXA255_DCMD6:		case PXA255_DCMD7:
		case PXA255_DCMD8:		case PXA255_DCMD9:		case PXA255_DCMD10:		case PXA255_DCMD11:
		case PXA255_DCMD12:		case PXA255_DCMD13:		case PXA255_DCMD14:		case PXA255_DCMD15:
			verboselog( space->machine, 0, "pxa255_dma_r: DMA Command Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, 0, mem_mask );
			return dma_regs.dcmd[(offset - (0x200 >> 2)) >> 2];
		default:
			verboselog( space->machine, 0, "pxa255_dma_r: Unknown address: %08x\n", PXA255_DMA_BASE_ADDR | (offset << 2));
			break;
	}
	return 0;
}

static WRITE32_HANDLER( pxa255_dma_w )
{
	switch(PXA255_DMA_BASE_ADDR | (offset << 2))
	{
		case PXA255_DCSR0:		case PXA255_DCSR1:		case PXA255_DCSR2:		case PXA255_DCSR3:
		case PXA255_DCSR4:		case PXA255_DCSR5:		case PXA255_DCSR6:		case PXA255_DCSR7:
		case PXA255_DCSR8:		case PXA255_DCSR9:		case PXA255_DCSR10:		case PXA255_DCSR11:
		case PXA255_DCSR12:		case PXA255_DCSR13:		case PXA255_DCSR14:		case PXA255_DCSR15:
			verboselog( space->machine, 0, "pxa255_dma_w: DMA Channel Control/Status Register %d: %08x & %08x\n", offset, data, mem_mask );
			dma_regs.dcsr[offset] &= ~(data & 0x00000007);
			dma_regs.dcsr[offset] &= ~0xe0000000;
			dma_regs.dcsr[offset] |= data & 0xe0000000;
			if(data & PXA255_DCSR_RUN)
			{
				attotime period;

				// Shut down any transfers that are currently going on, software should be smart enough to check if a
				// transfer is running before starting another one on the same channel.
				if(timer_enabled(dma_regs.timer[offset]))
				{
					timer_adjust_oneshot(dma_regs.timer[offset], attotime_never, 0);
				}

				if(data & PXA255_DCSR_NODESCFETCH)
				{
					verboselog( space->machine, 0, "              No-Descriptor-Fetch mode is not supported.\n" );
					break;
				}

				// Load the next descriptor
				dma_regs.dsadr[offset] = memory_read_dword_32le(space, dma_regs.ddadr[offset] + 0x4);
				dma_regs.dtadr[offset] = memory_read_dword_32le(space, dma_regs.ddadr[offset] + 0x8);
				dma_regs.dcmd[offset] = memory_read_dword_32le(space, dma_regs.ddadr[offset] + 0xc);
				dma_regs.ddadr[offset] = memory_read_dword_32le(space, dma_regs.ddadr[offset]);

				// Start our end-of-transfer timer
				period = attotime_mul(ATTOTIME_IN_HZ(200000000), dma_regs.dcmd[offset] & 0x00001fff);

				timer_adjust_oneshot(dma_regs.timer[offset], period, offset);

				// Interrupt as necessary
				if(dma_regs.dcmd[offset] & PXA255_DCMD_STARTIRQEN)
				{
					dma_regs.dcsr[offset] |= PXA255_DCSR_STARTINTR;
				}

				dma_regs.dcsr[offset] &= ~PXA255_DCSR_STOPSTATE;
			}
			pxa255_dma_irq_check(space->machine);
			break;
		case PXA255_DINT:
			verboselog( space->machine, 0, "pxa255_dma_w: DMA Interrupt Register: %08x & %08x\n", data, mem_mask );
			dma_regs.dint &= ~data;
			break;
		case PXA255_DRCMR0:		case PXA255_DRCMR1:		case PXA255_DRCMR2:		case PXA255_DRCMR3:
		case PXA255_DRCMR4:		case PXA255_DRCMR5:		case PXA255_DRCMR6:		case PXA255_DRCMR7:
		case PXA255_DRCMR8:		case PXA255_DRCMR9:		case PXA255_DRCMR10:	case PXA255_DRCMR11:
		case PXA255_DRCMR12:	case PXA255_DRCMR13:	case PXA255_DRCMR14:	case PXA255_DRCMR15:
		case PXA255_DRCMR16:	case PXA255_DRCMR17:	case PXA255_DRCMR18:	case PXA255_DRCMR19:
		case PXA255_DRCMR20:	case PXA255_DRCMR21:	case PXA255_DRCMR22:	case PXA255_DRCMR23:
		case PXA255_DRCMR24:	case PXA255_DRCMR25:	case PXA255_DRCMR26:	case PXA255_DRCMR27:
		case PXA255_DRCMR28:	case PXA255_DRCMR29:	case PXA255_DRCMR30:	case PXA255_DRCMR31:
		case PXA255_DRCMR32:	case PXA255_DRCMR33:	case PXA255_DRCMR34:	case PXA255_DRCMR35:
		case PXA255_DRCMR36:	case PXA255_DRCMR37:	case PXA255_DRCMR38:	case PXA255_DRCMR39:
			verboselog( space->machine, 0, "pxa255_dma_w: DMA Request to Channel Map Register %d: %08x & %08x\n", offset - (0x100 >> 2), data, mem_mask );
			dma_regs.drcmr[offset - (0x100 >> 2)] = data & 0x0000008f;
			break;
		case PXA255_DDADR0:		case PXA255_DDADR1:		case PXA255_DDADR2:		case PXA255_DDADR3:
		case PXA255_DDADR4:		case PXA255_DDADR5:		case PXA255_DDADR6:		case PXA255_DDADR7:
		case PXA255_DDADR8:		case PXA255_DDADR9:		case PXA255_DDADR10:	case PXA255_DDADR11:
		case PXA255_DDADR12:	case PXA255_DDADR13:	case PXA255_DDADR14:	case PXA255_DDADR15:
			verboselog( space->machine, 0, "pxa255_dma_w: DMA Descriptor Address Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, data, mem_mask );
			dma_regs.ddadr[(offset - (0x200 >> 2)) >> 2] = data & 0xfffffff1;
			break;
		case PXA255_DSADR0:		case PXA255_DSADR1:		case PXA255_DSADR2:		case PXA255_DSADR3:
		case PXA255_DSADR4:		case PXA255_DSADR5:		case PXA255_DSADR6:		case PXA255_DSADR7:
		case PXA255_DSADR8:		case PXA255_DSADR9:		case PXA255_DSADR10:	case PXA255_DSADR11:
		case PXA255_DSADR12:	case PXA255_DSADR13:	case PXA255_DSADR14:	case PXA255_DSADR15:
			verboselog( space->machine, 0, "pxa255_dma_w: DMA Source Address Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, data, mem_mask );
			dma_regs.dsadr[(offset - (0x200 >> 2)) >> 2] = data & 0xfffffffc;
			break;
		case PXA255_DTADR0:		case PXA255_DTADR1:		case PXA255_DTADR2:		case PXA255_DTADR3:
		case PXA255_DTADR4:		case PXA255_DTADR5:		case PXA255_DTADR6:		case PXA255_DTADR7:
		case PXA255_DTADR8:		case PXA255_DTADR9:		case PXA255_DTADR10:	case PXA255_DTADR11:
		case PXA255_DTADR12:	case PXA255_DTADR13:	case PXA255_DTADR14:	case PXA255_DTADR15:
			verboselog( space->machine, 0, "pxa255_dma_w: DMA Target Address Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, data, mem_mask );
			dma_regs.dtadr[(offset - (0x200 >> 2)) >> 2] = data & 0xfffffffc;
			break;
		case PXA255_DCMD0:		case PXA255_DCMD1:		case PXA255_DCMD2:		case PXA255_DCMD3:
		case PXA255_DCMD4:		case PXA255_DCMD5:		case PXA255_DCMD6:		case PXA255_DCMD7:
		case PXA255_DCMD8:		case PXA255_DCMD9:		case PXA255_DCMD10:		case PXA255_DCMD11:
		case PXA255_DCMD12:		case PXA255_DCMD13:		case PXA255_DCMD14:		case PXA255_DCMD15:
			verboselog( space->machine, 0, "pxa255_dma_w: DMA Command Register %d: %08x & %08x\n", (offset - (0x200 >> 2)) >> 2, data, mem_mask );
			dma_regs.dcmd[(offset - (0x200 >> 2)) >> 2] = data & 0xf067dfff;
			break;
		default:
			verboselog( space->machine, 0, "pxa255_dma_w: Unknown address: %08x = %08x & %08x\n", PXA255_DMA_BASE_ADDR | (offset << 2), data, mem_mask);
			break;
	}
}

/*

  PXA255 OS Timer register

  pg. 138 to 142, PXA255 Processor Developers Manual [278693-002].pdf

*/

static void pxa255_ostimer_irq_check(running_machine* machine)
{
	pxa255_set_irq_line(machine, PXA255_INT_OSTIMER0, (ostimer_regs.ossr & PXA255_OSSR_M0) ? 1 : 0);
	pxa255_set_irq_line(machine, PXA255_INT_OSTIMER1, (ostimer_regs.ossr & PXA255_OSSR_M1) ? 1 : 0);
	pxa255_set_irq_line(machine, PXA255_INT_OSTIMER2, (ostimer_regs.ossr & PXA255_OSSR_M2) ? 1 : 0);
	pxa255_set_irq_line(machine, PXA255_INT_OSTIMER3, (ostimer_regs.ossr & PXA255_OSSR_M3) ? 1 : 0);
}

static TIMER_CALLBACK( pxa255_ostimer_match )
{
	ostimer_regs.ossr |= (1 << param);
	pxa255_ostimer_irq_check(machine);
}

static READ32_HANDLER( pxa255_ostimer_r )
{
	switch(PXA255_OSTMR_BASE_ADDR | (offset << 2))
	{
		case PXA255_OSMR0:
			verboselog( space->machine, 3, "pxa255_ostimer_r: OS Timer Match Register 0: %08x & %08x\n", ostimer_regs.osmr[0], mem_mask );
			return ostimer_regs.osmr[0];
		case PXA255_OSMR1:
			verboselog( space->machine, 3, "pxa255_ostimer_r: OS Timer Match Register 1: %08x & %08x\n", ostimer_regs.osmr[1], mem_mask );
			return ostimer_regs.osmr[1];
		case PXA255_OSMR2:
			verboselog( space->machine, 3, "pxa255_ostimer_r: OS Timer Match Register 2: %08x & %08x\n", ostimer_regs.osmr[2], mem_mask );
			return ostimer_regs.osmr[2];
		case PXA255_OSMR3:
			verboselog( space->machine, 3, "pxa255_ostimer_r: OS Timer Match Register 3: %08x & %08x\n", ostimer_regs.osmr[3], mem_mask );
			return ostimer_regs.osmr[3];
		case PXA255_OSCR:
			verboselog( space->machine, 4, "pxa255_ostimer_r: OS Timer Count Register: %08x & %08x\n", ostimer_regs.oscr, mem_mask );
			// free-running 3.something MHz counter.  this is a complete hack.
			ostimer_regs.oscr += 0x300;
			return ostimer_regs.oscr;
		case PXA255_OSSR:
			verboselog( space->machine, 3, "pxa255_ostimer_r: OS Timer Status Register: %08x & %08x\n", ostimer_regs.ossr, mem_mask );
			return ostimer_regs.ossr;
		case PXA255_OWER:
			verboselog( space->machine, 3, "pxa255_ostimer_r: OS Timer Watchdog Match Enable Register: %08x & %08x\n", ostimer_regs.ower, mem_mask );
			return ostimer_regs.ower;
		case PXA255_OIER:
			verboselog( space->machine, 3, "pxa255_ostimer_r: OS Timer Interrupt Enable Register: %08x & %08x\n", ostimer_regs.oier, mem_mask );
			return ostimer_regs.oier;
		default:
			verboselog( space->machine, 0, "pxa255_ostimer_r: Unknown address: %08x\n", PXA255_OSTMR_BASE_ADDR | (offset << 2));
			break;
	}
	return 0;
}

static WRITE32_HANDLER( pxa255_ostimer_w )
{
	switch(PXA255_OSTMR_BASE_ADDR | (offset << 2))
	{
		case PXA255_OSMR0:
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Match Register 0: %08x & %08x\n", data, mem_mask );
			ostimer_regs.osmr[0] = data;
			if(ostimer_regs.oier & PXA255_OIER_E0)
			{
				attotime period = attotime_mul(ATTOTIME_IN_HZ(200000000), ostimer_regs.osmr[0]);

				timer_adjust_oneshot(ostimer_regs.timer[0], period, 0);
			}
			break;
		case PXA255_OSMR1:
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Match Register 1: %08x & %08x\n", data, mem_mask );
			ostimer_regs.osmr[1] = data;
			if(ostimer_regs.oier & PXA255_OIER_E1)
			{
				attotime period = attotime_mul(ATTOTIME_IN_HZ(200000000), ostimer_regs.osmr[1]);

				timer_adjust_oneshot(ostimer_regs.timer[1], period, 1);
			}
			break;
		case PXA255_OSMR2:
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Match Register 2: %08x & %08x\n", data, mem_mask );
			ostimer_regs.osmr[2] = data;
			if(ostimer_regs.oier & PXA255_OIER_E2)
			{
				attotime period = attotime_mul(ATTOTIME_IN_HZ(200000000), ostimer_regs.osmr[2]);

				timer_adjust_oneshot(ostimer_regs.timer[2], period, 2);
			}
			break;
		case PXA255_OSMR3:
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Match Register 3: %08x & %08x\n", data, mem_mask );
			ostimer_regs.osmr[3] = data;
			if(ostimer_regs.oier & PXA255_OIER_E3)
			{
				attotime period = attotime_mul(ATTOTIME_IN_HZ(200000000), ostimer_regs.osmr[3]);

				timer_adjust_oneshot(ostimer_regs.timer[3], period, 3);
			}
			break;
		case PXA255_OSCR:
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Count Register: %08x & %08x\n", data, mem_mask );
			ostimer_regs.oscr = data;
			break;
		case PXA255_OSSR:
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Status Register: %08x & %08x\n", data, mem_mask );
			ostimer_regs.ossr &= ~data;
			pxa255_ostimer_irq_check(space->machine);
			break;
		case PXA255_OWER:
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Watchdog Enable Register: %08x & %08x\n", data, mem_mask );
			ostimer_regs.ower = data & 0x00000001;
			break;
		case PXA255_OIER:
		{
			int index = 0;
			verboselog( space->machine, 3, "pxa255_ostimer_w: OS Timer Interrupt Enable Register: %08x & %08x\n", data, mem_mask );
			ostimer_regs.oier = data & 0x0000000f;
			for(index = 0; index < 4; index++)
			{
				if(ostimer_regs.oier & (1 << index))
				{
					attotime period = attotime_mul(ATTOTIME_IN_HZ(200000000), ostimer_regs.osmr[index]);

					timer_adjust_oneshot(ostimer_regs.timer[index], period, index);
				}
			}

			break;
		}
		default:
			verboselog( space->machine, 0, "pxa255_ostimer_w: Unknown address: %08x = %08x & %08x\n", PXA255_OSTMR_BASE_ADDR | (offset << 2), data, mem_mask);
			break;
	}
}

/*

  PXA255 Interrupt registers

  pg. 124 to 132, PXA255 Processor Developers Manual [278693-002].pdf

*/

static void pxa255_update_interrupts(running_machine* machine)
{
	intc_regs.icfp = (intc_regs.icpr & intc_regs.icmr) & intc_regs.iclr;
	intc_regs.icip = (intc_regs.icpr & intc_regs.icmr) & (~intc_regs.iclr);
	cputag_set_input_line(machine, "maincpu", ARM7_FIRQ_LINE, intc_regs.icfp ? ASSERT_LINE : CLEAR_LINE);
	cputag_set_input_line(machine, "maincpu", ARM7_IRQ_LINE,  intc_regs.icip ? ASSERT_LINE : CLEAR_LINE);
}

static void pxa255_set_irq_line(running_machine* machine, UINT32 line, int state)
{
	intc_regs.icpr &= ~line;
	intc_regs.icpr |= state ? line : 0;
	pxa255_update_interrupts(machine);
}

static READ32_HANDLER( pxa255_intc_r )
{
	switch(PXA255_INTC_BASE_ADDR | (offset << 2))
	{
		case PXA255_ICIP:
			verboselog( space->machine, 3, "pxa255_intc_r: Interrupt Controller IRQ Pending Register: %08x & %08x\n", intc_regs.icip, mem_mask );
			return intc_regs.icip;
		case PXA255_ICMR:
			verboselog( space->machine, 3, "pxa255_intc_r: Interrupt Controller Mask Register: %08x & %08x\n", intc_regs.icmr, mem_mask );
			return intc_regs.icmr;
		case PXA255_ICLR:
			verboselog( space->machine, 3, "pxa255_intc_r: Interrupt Controller Level Register: %08x & %08x\n", intc_regs.iclr, mem_mask );
			return intc_regs.iclr;
		case PXA255_ICFP:
			verboselog( space->machine, 3, "pxa255_intc_r: Interrupt Controller FIQ Pending Register: %08x & %08x\n", intc_regs.icfp, mem_mask );
			return intc_regs.icfp;
		case PXA255_ICPR:
			verboselog( space->machine, 3, "pxa255_intc_r: Interrupt Controller Pending Register: %08x & %08x\n", intc_regs.icpr, mem_mask );
			return intc_regs.icpr;
		case PXA255_ICCR:
			verboselog( space->machine, 3, "pxa255_intc_r: Interrupt Controller Control Register: %08x & %08x\n", intc_regs.iccr, mem_mask );
			return intc_regs.iccr;
		default:
			verboselog( space->machine, 0, "pxa255_intc_r: Unknown address: %08x\n", PXA255_INTC_BASE_ADDR | (offset << 2));
			break;
	}
	return 0;
}

static WRITE32_HANDLER( pxa255_intc_w )
{
	switch(PXA255_INTC_BASE_ADDR | (offset << 2))
	{
		case PXA255_ICIP:
			verboselog( space->machine, 3, "pxa255_intc_w: (Invalid Write) Interrupt Controller IRQ Pending Register: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_ICMR:
			verboselog( space->machine, 3, "pxa255_intc_w: Interrupt Controller Mask Register: %08x & %08x\n", data, mem_mask );
			intc_regs.icmr = data & 0xfffe7f00;
			break;
		case PXA255_ICLR:
			verboselog( space->machine, 3, "pxa255_intc_w: Interrupt Controller Level Register: %08x & %08x\n", data, mem_mask );
			intc_regs.iclr = data & 0xfffe7f00;
			break;
		case PXA255_ICFP:
			verboselog( space->machine, 3, "pxa255_intc_w: (Invalid Write) Interrupt Controller FIQ Pending Register: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_ICPR:
			verboselog( space->machine, 3, "pxa255_intc_w: (Invalid Write) Interrupt Controller Pending Register: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_ICCR:
			verboselog( space->machine, 3, "pxa255_intc_w: Interrupt Controller Control Register: %08x & %08x\n", data, mem_mask );
			intc_regs.iccr = data & 0x00000001;
			break;
		default:
			verboselog( space->machine, 0, "pxa255_intc_w: Unknown address: %08x = %08x & %08x\n", PXA255_INTC_BASE_ADDR | (offset << 2), data, mem_mask);
			break;
	}
}

/*

  PXA255 General-Purpose I/O registers

  pg. 105 to 124, PXA255 Processor Developers Manual [278693-002].pdf

*/

static READ32_HANDLER( pxa255_gpio_r )
{
	switch(PXA255_GPIO_BASE_ADDR | (offset << 2))
	{
		case PXA255_GPLR0:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Pin-Level Register 0: %08x & %08x\n", gpio_regs.gplr0 | (1 << 1), mem_mask );
			return gpio_regs.gplr0 | (1 << 1) | (eeprom_read_bit() << 5); // Must be on.  Probably a DIP switch.
		case PXA255_GPLR1:
			verboselog( space->machine, 3, "pxa255_gpio_r: *Not Yet Implemented* GPIO Pin-Level Register 1: %08x & %08x\n", gpio_regs.gplr1, mem_mask );
			return gpio_regs.gplr1;
		case PXA255_GPLR2:
			verboselog( space->machine, 3, "pxa255_gpio_r: *Not Yet Implemented* GPIO Pin-Level Register 2: %08x & %08x\n", gpio_regs.gplr2, mem_mask );
			return gpio_regs.gplr2;
		case PXA255_GPDR0:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Pin Direction Register 0: %08x & %08x\n", gpio_regs.gpdr0, mem_mask );
			return gpio_regs.gpdr0;
		case PXA255_GPDR1:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Pin Direction Register 1: %08x & %08x\n", gpio_regs.gpdr1, mem_mask );
			return gpio_regs.gpdr1;
		case PXA255_GPDR2:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Pin Direction Register 2: %08x & %08x\n", gpio_regs.gpdr2, mem_mask );
			return gpio_regs.gpdr2;
		case PXA255_GPSR0:
			verboselog( space->machine, 3, "pxa255_gpio_r: (Invalid Read) GPIO Pin Output Set Register 0: %08x & %08x\n", mame_rand(space->machine), mem_mask );
			return mame_rand(space->machine);
		case PXA255_GPSR1:
			verboselog( space->machine, 3, "pxa255_gpio_r: (Invalid Read) GPIO Pin Output Set Register 1: %08x & %08x\n", mame_rand(space->machine), mem_mask );
			return mame_rand(space->machine);
		case PXA255_GPSR2:
			verboselog( space->machine, 3, "pxa255_gpio_r: (Invalid Read) GPIO Pin Output Set Register 2: %08x & %08x\n", mame_rand(space->machine), mem_mask );
			return mame_rand(space->machine);
		case PXA255_GPCR0:
			verboselog( space->machine, 3, "pxa255_gpio_r: (Invalid Read) GPIO Pin Output Clear Register 0: %08x & %08x\n", mame_rand(space->machine), mem_mask );
			return mame_rand(space->machine);
		case PXA255_GPCR1:
			verboselog( space->machine, 3, "pxa255_gpio_r: (Invalid Read) GPIO Pin Output Clear Register 1: %08x & %08x\n", mame_rand(space->machine), mem_mask );
			return mame_rand(space->machine);
		case PXA255_GPCR2:
			verboselog( space->machine, 3, "pxa255_gpio_r: (Invalid Read) GPIO Pin Output Clear Register 2: %08x & %08x\n", mame_rand(space->machine), mem_mask );
			return mame_rand(space->machine);
		case PXA255_GRER0:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Rising Edge Detect Enable Register 0: %08x & %08x\n", gpio_regs.grer0, mem_mask );
			return gpio_regs.grer0;
		case PXA255_GRER1:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Rising Edge Detect Enable Register 1: %08x & %08x\n", gpio_regs.grer1, mem_mask );
			return gpio_regs.grer1;
		case PXA255_GRER2:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Rising Edge Detect Enable Register 2: %08x & %08x\n", gpio_regs.grer2, mem_mask );
			return gpio_regs.grer2;
		case PXA255_GFER0:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Falling Edge Detect Enable Register 0: %08x & %08x\n", gpio_regs.gfer0, mem_mask );
			return gpio_regs.gfer0;
		case PXA255_GFER1:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Falling Edge Detect Enable Register 1: %08x & %08x\n", gpio_regs.gfer1, mem_mask );
			return gpio_regs.gfer1;
		case PXA255_GFER2:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Falling Edge Detect Enable Register 2: %08x & %08x\n", gpio_regs.gfer2, mem_mask );
			return gpio_regs.gfer2;
		case PXA255_GEDR0:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Edge Detect Status Register 0: %08x & %08x\n", gpio_regs.gedr0, mem_mask );
			return gpio_regs.gedr0;
		case PXA255_GEDR1:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Edge Detect Status Register 1: %08x & %08x\n", gpio_regs.gedr1, mem_mask );
			return gpio_regs.gedr1;
		case PXA255_GEDR2:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Edge Detect Status Register 2: %08x & %08x\n", gpio_regs.gedr2, mem_mask );
			return gpio_regs.gedr2;
		case PXA255_GAFR0_L:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Alternate Function Register 0 Lower: %08x & %08x\n", gpio_regs.gafr0l, mem_mask );
			return gpio_regs.gafr0l;
		case PXA255_GAFR0_U:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Alternate Function Register 0 Upper: %08x & %08x\n", gpio_regs.gafr0u, mem_mask );
			return gpio_regs.gafr0u;
		case PXA255_GAFR1_L:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Alternate Function Register 1 Lower: %08x & %08x\n", gpio_regs.gafr1l, mem_mask );
			return gpio_regs.gafr1l;
		case PXA255_GAFR1_U:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Alternate Function Register 1 Upper: %08x & %08x\n", gpio_regs.gafr1u, mem_mask );
			return gpio_regs.gafr1u;
		case PXA255_GAFR2_L:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Alternate Function Register 2 Lower: %08x & %08x\n", gpio_regs.gafr2l, mem_mask );
			return gpio_regs.gafr2l;
		case PXA255_GAFR2_U:
			verboselog( space->machine, 3, "pxa255_gpio_r: GPIO Alternate Function Register 2 Upper: %08x & %08x\n", gpio_regs.gafr2u, mem_mask );
			return gpio_regs.gafr2u;
		default:
			verboselog( space->machine, 0, "pxa255_gpio_r: Unknown address: %08x\n", PXA255_GPIO_BASE_ADDR | (offset << 2));
			break;
	}
	return 0;
}

static WRITE32_HANDLER( pxa255_gpio_w )
{
	switch(PXA255_GPIO_BASE_ADDR | (offset << 2))
	{
		case PXA255_GPLR0:
			verboselog( space->machine, 3, "pxa255_gpio_w: (Invalid Write) GPIO Pin-Level Register 0: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_GPLR1:
			verboselog( space->machine, 3, "pxa255_gpio_w: (Invalid Write) GPIO Pin-Level Register 1: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_GPLR2:
			verboselog( space->machine, 3, "pxa255_gpio_w: (Invalid Write) GPIO Pin-Level Register 2: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_GPDR0:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Direction Register 0: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpdr0 = data;
			break;
		case PXA255_GPDR1:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Direction Register 1: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpdr1 = data;
			break;
		case PXA255_GPDR2:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Direction Register 2: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpdr2 = data;
			break;
		case PXA255_GPSR0:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Output Set Register 0: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpsr0 |= data & gpio_regs.gpdr0;
			if(data & 0x00000004)
			{
				eeprom_set_cs_line(ASSERT_LINE);
			}
			if(data & 0x00000008)
			{
				eeprom_set_clock_line(ASSERT_LINE);
			}
			if(data & 0x00000010)
			{
				eeprom_write_bit(1);
			}
			break;
		case PXA255_GPSR1:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Output Set Register 1: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpsr1 |= data & gpio_regs.gpdr1;
			break;
		case PXA255_GPSR2:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Output Set Register 2: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpsr2 |= data & gpio_regs.gpdr2;
			break;
		case PXA255_GPCR0:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Output Clear Register 0: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpsr0 &= ~(data & gpio_regs.gpdr0);
			if(data & 0x00000004)
			{
				eeprom_set_cs_line(CLEAR_LINE);
			}
			if(data & 0x00000008)
			{
				eeprom_set_clock_line(CLEAR_LINE);
			}
			if(data & 0x00000010)
			{
				eeprom_write_bit(0);
			}
			break;
		case PXA255_GPCR1:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Output Clear Register 1: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpsr1 &= ~(data & gpio_regs.gpdr1);
			break;
		case PXA255_GPCR2:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Pin Output Clear Register 2: %08x & %08x\n", data, mem_mask );
			gpio_regs.gpsr2 &= ~(data & gpio_regs.gpdr2);
			break;
		case PXA255_GRER0:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Rising Edge Detect Enable Register 0: %08x & %08x\n", data, mem_mask );
			gpio_regs.grer0 = data;
			break;
		case PXA255_GRER1:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Rising Edge Detect Enable Register 1: %08x & %08x\n", data, mem_mask );
			gpio_regs.grer1 = data;
			break;
		case PXA255_GRER2:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Rising Edge Detect Enable Register 2: %08x & %08x\n", data, mem_mask );
			gpio_regs.grer2 = data;
			break;
		case PXA255_GFER0:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Falling Edge Detect Enable Register 0: %08x & %08x\n", data, mem_mask );
			gpio_regs.gfer0 = data;
			break;
		case PXA255_GFER1:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Falling Edge Detect Enable Register 1: %08x & %08x\n", data, mem_mask );
			gpio_regs.gfer1 = data;
			break;
		case PXA255_GFER2:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Falling Edge Detect Enable Register 2: %08x & %08x\n", data, mem_mask );
			gpio_regs.gfer2 = data;
			break;
		case PXA255_GEDR0:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Edge Detect Status Register 0: %08x & %08x\n", gpio_regs.gedr0, mem_mask );
			gpio_regs.gedr0 &= ~data;
			break;
		case PXA255_GEDR1:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Edge Detect Status Register 1: %08x & %08x\n", gpio_regs.gedr1, mem_mask );
			gpio_regs.gedr1 &= ~data;
			break;
		case PXA255_GEDR2:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Edge Detect Status Register 2: %08x & %08x\n", gpio_regs.gedr2, mem_mask );
			gpio_regs.gedr2 &= ~data;
			break;
		case PXA255_GAFR0_L:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Alternate Function Register 0 Lower: %08x & %08x\n", gpio_regs.gafr0l, mem_mask );
			gpio_regs.gafr0l = data;
			break;
		case PXA255_GAFR0_U:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Alternate Function Register 0 Upper: %08x & %08x\n", gpio_regs.gafr0u, mem_mask );
			gpio_regs.gafr0u = data;
			break;
		case PXA255_GAFR1_L:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Alternate Function Register 1 Lower: %08x & %08x\n", gpio_regs.gafr1l, mem_mask );
			gpio_regs.gafr1l = data;
			break;
		case PXA255_GAFR1_U:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Alternate Function Register 1 Upper: %08x & %08x\n", gpio_regs.gafr1u, mem_mask );
			gpio_regs.gafr1u = data;
			break;
		case PXA255_GAFR2_L:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Alternate Function Register 2 Lower: %08x & %08x\n", gpio_regs.gafr2l, mem_mask );
			gpio_regs.gafr2l = data;
			break;
		case PXA255_GAFR2_U:
			verboselog( space->machine, 3, "pxa255_gpio_w: GPIO Alternate Function Register 2 Upper: %08x & %08x\n", gpio_regs.gafr2u, mem_mask );
			gpio_regs.gafr2u = data;
			break;
		default:
			verboselog( space->machine, 0, "pxa255_gpio_w: Unknown address: %08x = %08x & %08x\n", PXA255_GPIO_BASE_ADDR | (offset << 2), data, mem_mask);
			break;
	}
}

/*

  PXA255 LCD Controller

  pg. 265 to 310, PXA255 Processor Developers Manual [278693-002].pdf

*/

static void pxa255_lcd_load_dma_descriptor(const address_space* space, UINT32 address, int channel)
{
	lcd_regs.dma[channel].fdadr = memory_read_dword_32le(space, address);
	lcd_regs.dma[channel].fsadr = memory_read_dword_32le(space, address + 0x04);
	lcd_regs.dma[channel].fidr  = memory_read_dword_32le(space, address + 0x08);
	lcd_regs.dma[channel].ldcmd = memory_read_dword_32le(space, address + 0x0c);
	verboselog( space->machine, 3, "pxa255_lcd_load_dma_descriptor, address = %08x, channel = %d\n", address, channel);
	verboselog( space->machine, 3, "    DMA Frame Descriptor: %08x\n", lcd_regs.dma[channel].fdadr );
	verboselog( space->machine, 3, "    DMA Frame Source Address: %08x\n", lcd_regs.dma[channel].fsadr );
	verboselog( space->machine, 3, "    DMA Frame ID: %08x\n", lcd_regs.dma[channel].fidr );
	verboselog( space->machine, 3, "    DMA Command: %08x\n", lcd_regs.dma[channel].ldcmd );
}

static void pxa255_lcd_irq_check(running_machine* machine)
{
	if(((lcd_regs.lcsr & PXA255_LCSR_BS)  != 0 && (lcd_regs.lccr0 & PXA255_LCCR0_BM)  == 0) ||
	   ((lcd_regs.lcsr & PXA255_LCSR_EOF) != 0 && (lcd_regs.lccr0 & PXA255_LCCR0_EFM) == 0) ||
	   ((lcd_regs.lcsr & PXA255_LCSR_SOF) != 0 && (lcd_regs.lccr0 & PXA255_LCCR0_SFM) == 0))
	{
		pxa255_set_irq_line(machine, PXA255_INT_LCD, 1);
	}
	else
	{
		pxa255_set_irq_line(machine, PXA255_INT_LCD, 0);
	}
}

static void pxa255_lcd_dma_kickoff(running_machine* machine, int channel)
{
	if(lcd_regs.dma[channel].fdadr != 0)
	{
		attotime period = attotime_mul(ATTOTIME_IN_HZ(200000000), lcd_regs.dma[channel].ldcmd & 0x000fffff);

		timer_adjust_oneshot(lcd_regs.dma[channel].eof, period, channel);

		if(lcd_regs.dma[channel].ldcmd & PXA255_LDCMD_SOFINT)
		{
			lcd_regs.liidr = lcd_regs.dma[channel].fidr;
			lcd_regs.lcsr |= PXA255_LCSR_SOF;
			pxa255_lcd_irq_check(machine);
		}
	}
}

static void pxa255_lcd_check_load_next_branch(running_machine* machine, int channel)
{
	if(lcd_regs.fbr[channel] & 1)
	{
		lcd_regs.fbr[channel] &= ~1;
		//printf( "%08x\n", memory_read_dword_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), lcd_regs.fbr[channel] & 0xfffffff0) );
		lcd_regs.fbr[channel] |= memory_read_dword_32le(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), lcd_regs.fbr[channel] & 0xfffffff0) & 0xfffffff0;
		pxa255_lcd_load_dma_descriptor(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), lcd_regs.fbr[channel] & 0xfffffff0, 0);
		pxa255_lcd_dma_kickoff(machine, 0);
		if(!(lcd_regs.lccr0 & PXA255_LCCR0_BM))
		{
			lcd_regs.lcsr |= PXA255_LCSR_BS;
		}
	}
}

static TIMER_CALLBACK( pxa255_lcd_dma_eof )
{
	if(lcd_regs.dma[param].ldcmd & PXA255_LDCMD_EOFINT)
	{
		lcd_regs.liidr = lcd_regs.dma[param].fidr;
		lcd_regs.lcsr |= PXA255_LCSR_EOF;
	}
	pxa255_lcd_check_load_next_branch(machine, param);
}

static READ32_HANDLER( pxa255_lcd_r )
{
	switch(PXA255_LCD_BASE_ADDR | (offset << 2))
	{
		case PXA255_LCCR0:		// 0x44000000
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Control 0: %08x & %08x\n", lcd_regs.lccr0, mem_mask );
			return lcd_regs.lccr0;
		case PXA255_LCCR1:		// 0x44000004
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Control 1: %08x & %08x\n", lcd_regs.lccr1, mem_mask );
			return lcd_regs.lccr1;
		case PXA255_LCCR2:		// 0x44000008
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Control 2: %08x & %08x\n", lcd_regs.lccr2, mem_mask );
			return lcd_regs.lccr2;
		case PXA255_LCCR3:		// 0x4400000c
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Control 3: %08x & %08x\n", lcd_regs.lccr3, mem_mask );
			return lcd_regs.lccr3;
		case PXA255_FBR0:		// 0x44000020
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Frame Branch Register 0: %08x & %08x\n", lcd_regs.fbr[0], mem_mask );
			return lcd_regs.fbr[0];
		case PXA255_FBR1:		// 0x44000024
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Frame Branch Register 1: %08x & %08x\n", lcd_regs.fbr[1], mem_mask );
			return lcd_regs.fbr[1];
		case PXA255_LCSR:		// 0x44000038
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Status Register: %08x & %08x\n", lcd_regs.lcsr, mem_mask );
			return lcd_regs.lcsr;
		case PXA255_LIIDR:		// 0x4400003c
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD Interrupt ID Register: %08x & %08x\n", lcd_regs.liidr, mem_mask );
			return lcd_regs.liidr;
		case PXA255_TRGBR:		// 0x44000040
			verboselog( space->machine, 3, "pxa255_lcd_r: TMED RGB Seed Register: %08x & %08x\n", lcd_regs.trgbr, mem_mask );
			return lcd_regs.trgbr;
		case PXA255_TCR:		// 0x44000044
			verboselog( space->machine, 3, "pxa255_lcd_r: TMED RGB Seed Register: %08x & %08x\n", lcd_regs.tcr, mem_mask );
			return lcd_regs.tcr;
		case PXA255_FDADR0:		// 0x44000200
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Frame Descriptor Address Register 0: %08x & %08x\n", lcd_regs.dma[0].fdadr, mem_mask );
			return lcd_regs.dma[0].fdadr;
		case PXA255_FSADR0:		// 0x44000204
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Frame Source Address Register 0: %08x & %08x\n", lcd_regs.dma[0].fsadr, mem_mask );
			return lcd_regs.dma[0].fsadr;
		case PXA255_FIDR0:		// 0x44000208
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Frame ID Register 0: %08x & %08x\n", lcd_regs.dma[0].fidr, mem_mask );
			return lcd_regs.dma[0].fidr;
		case PXA255_LDCMD0:		// 0x4400020c
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Command Register 0: %08x & %08x\n", lcd_regs.dma[0].ldcmd & 0xfff00000, mem_mask );
			return lcd_regs.dma[0].ldcmd & 0xfff00000;
		case PXA255_FDADR1:		// 0x44000210
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Frame Descriptor Address Register 1: %08x & %08x\n", lcd_regs.dma[1].fdadr, mem_mask );
			return lcd_regs.dma[1].fdadr;
		case PXA255_FSADR1:		// 0x44000214
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Frame Source Address Register 1: %08x & %08x\n", lcd_regs.dma[1].fsadr, mem_mask );
			return lcd_regs.dma[1].fsadr;
		case PXA255_FIDR1:		// 0x44000218
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Frame ID Register 1: %08x & %08x\n", lcd_regs.dma[1].fidr, mem_mask );
			return lcd_regs.dma[1].fidr;
		case PXA255_LDCMD1:		// 0x4400021c
			verboselog( space->machine, 3, "pxa255_lcd_r: LCD DMA Command Register 1: %08x & %08x\n", lcd_regs.dma[1].ldcmd & 0xfff00000, mem_mask );
			return lcd_regs.dma[1].ldcmd & 0xfff00000;
		default:
			verboselog( space->machine, 0, "pxa255_lcd_r: Unknown address: %08x\n", PXA255_LCD_BASE_ADDR | (offset << 2));
			break;
	}
	return 0;
}

static WRITE32_HANDLER( pxa255_lcd_w )
{
	switch(PXA255_LCD_BASE_ADDR | (offset << 2))
	{
		case PXA255_LCCR0:		// 0x44000000
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Control 0: %08x & %08x\n", data, mem_mask );
			lcd_regs.lccr0 = data & 0x00fffeff;
			break;
		case PXA255_LCCR1:		// 0x44000004
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Control 1: %08x & %08x\n", data, mem_mask );
			lcd_regs.lccr1 = data;
			break;
		case PXA255_LCCR2:		// 0x44000008
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Control 2: %08x & %08x\n", data, mem_mask );
			lcd_regs.lccr2 = data;
			break;
		case PXA255_LCCR3:		// 0x4400000c
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Control 3: %08x & %08x\n", data, mem_mask );
			lcd_regs.lccr3 = data;
			break;
		case PXA255_FBR0:		// 0x44000020
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Frame Branch Register 0: %08x & %08x\n", data, mem_mask );
			lcd_regs.fbr[0] = data & 0xfffffff3;
			if(!timer_enabled(lcd_regs.dma[0].eof))
			{
				pxa255_lcd_check_load_next_branch(space->machine, 0);
				pxa255_lcd_irq_check(space->machine);
			}
			break;
		case PXA255_FBR1:		// 0x44000024
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Frame Branch Register 1: %08x & %08x\n", data, mem_mask );
			lcd_regs.fbr[1] = data & 0xfffffff3;
			if(!timer_enabled(lcd_regs.dma[1].eof))
			{
				pxa255_lcd_check_load_next_branch(space->machine, 1);
				pxa255_lcd_irq_check(space->machine);
			}
			break;
		case PXA255_LCSR:		// 0x44000038
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Controller Status Register: %08x & %08x\n", data, mem_mask );
			lcd_regs.lcsr &= ~data;
			pxa255_lcd_irq_check(space->machine);
			break;
		case PXA255_LIIDR:		// 0x4400003c
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD Controller Interrupt ID Register: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_TRGBR:		// 0x44000040
			verboselog( space->machine, 3, "pxa255_lcd_w: TMED RGB Seed Register: %08x & %08x\n", data, mem_mask );
			lcd_regs.trgbr = data & 0x00ffffff;
			break;
		case PXA255_TCR:		// 0x44000044
			verboselog( space->machine, 3, "pxa255_lcd_w: TMED Control Register: %08x & %08x\n", data, mem_mask );
			lcd_regs.tcr = data & 0x00004fff;
			break;
		case PXA255_FDADR0:		// 0x44000200
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD DMA Frame Descriptor Address Register 0: %08x & %08x\n", data, mem_mask );
			pxa255_lcd_load_dma_descriptor(space, data & 0xfffffff0, 0);
			break;
		case PXA255_FSADR0:		// 0x44000204
			verboselog( space->machine, 3, "pxa255_lcd_w: (Invalid Write) LCD DMA Frame Source Address Register 0: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_FIDR0:		// 0x44000208
			verboselog( space->machine, 3, "pxa255_lcd_w: (Invalid Write) LCD DMA Frame ID Register 0: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_LDCMD0:		// 0x4400020c
			verboselog( space->machine, 3, "pxa255_lcd_w: (Invalid Write) LCD DMA Command Register 0: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_FDADR1:		// 0x44000210
			verboselog( space->machine, 3, "pxa255_lcd_w: LCD DMA Frame Descriptor Address Register 1: %08x & %08x\n", data, mem_mask );
			pxa255_lcd_load_dma_descriptor(space, data & 0xfffffff0, 1);
			break;
		case PXA255_FSADR1:		// 0x44000214
			verboselog( space->machine, 3, "pxa255_lcd_w: (Invalid Write) LCD DMA Frame Source Address Register 1: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_FIDR1:		// 0x44000218
			verboselog( space->machine, 3, "pxa255_lcd_w: (Invalid Write) LCD DMA Frame ID Register 1: %08x & %08x\n", data, mem_mask );
			break;
		case PXA255_LDCMD1:		// 0x4400021c
			verboselog( space->machine, 3, "pxa255_lcd_w: (Invalid Write) LCD DMA Command Register 1: %08x & %08x\n", data, mem_mask );
			break;
		default:
			verboselog( space->machine, 0, "pxa255_lcd_w: Unknown address: %08x = %08x & %08x\n", PXA255_LCD_BASE_ADDR | (offset << 2), data, mem_mask);
			break;
	}
}

static INTERRUPT_GEN( pxa255_vblank_start )
{
}

static ADDRESS_MAP_START( 39in1_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_ROM
	AM_RANGE(0x40000000, 0x400002ff) AM_READWRITE( pxa255_dma_r, pxa255_dma_w )
	AM_RANGE(0x40400000, 0x40400083) AM_READWRITE( pxa255_i2s_r, pxa255_i2s_w )
	AM_RANGE(0x40a00000, 0x40a0001f) AM_READWRITE( pxa255_ostimer_r, pxa255_ostimer_w )
	AM_RANGE(0x40d00000, 0x40d00017) AM_READWRITE( pxa255_intc_r, pxa255_intc_w )
	AM_RANGE(0x40e00000, 0x40e0006b) AM_READWRITE( pxa255_gpio_r, pxa255_gpio_w )
	AM_RANGE(0x44000000, 0x4400021f) AM_READWRITE( pxa255_lcd_r,  pxa255_lcd_w )
	AM_RANGE(0xa0000000, 0xa07fffff) AM_RAM
ADDRESS_MAP_END

static INPUT_PORTS_START( 39in1 )
INPUT_PORTS_END

static VIDEO_UPDATE( 39in1 )
{
	return 0;
}

/* To be moved to DEVICE_START( pxa255 ) upon completion */
static void pxa255_start(running_machine* machine)
{
	int index = 0;

    //pxa255_t* pxa255 = pxa255_get_safe_token( device );

    //pxa255->iface = device->static_config;

	for(index = 0; index < 16; index++)
	{
		dma_regs.dcsr[index] = 0x00000008;
		dma_regs.timer[index] = timer_alloc(machine, pxa255_dma_dma_end, 0);
	}

	ostimer_regs.oscr = ostimer_regs.ossr = ostimer_regs.ower = ostimer_regs.oier = 0;
	for(index = 0; index < 4; index++)
	{
		ostimer_regs.osmr[index] = 0;
		ostimer_regs.timer[index] = timer_alloc(machine, pxa255_ostimer_match, 0);
	}

	intc_regs.icmr = intc_regs.iclr = intc_regs.iccr = intc_regs.icip = intc_regs.icfp = intc_regs.icpr = 0;

	lcd_regs.lccr0 = lcd_regs.lccr1 = lcd_regs.lccr2 = lcd_regs.lccr3 = 0;
	lcd_regs.dma[0].fdadr = lcd_regs.dma[1].fdadr = 0;
	lcd_regs.dma[0].fsadr = lcd_regs.dma[1].fsadr = 0;
	lcd_regs.dma[0].fidr  = lcd_regs.dma[1].fidr  = 0;
	lcd_regs.dma[0].ldcmd = lcd_regs.dma[1].ldcmd = 0;
	lcd_regs.dma[0].eof = timer_alloc(machine, pxa255_lcd_dma_eof, 0);
	lcd_regs.dma[1].eof = timer_alloc(machine, pxa255_lcd_dma_eof, 0);
	lcd_regs.fbr[0] = lcd_regs.fbr[1] = lcd_regs.lcsr = lcd_regs.liidr = 0;
	lcd_regs.trgbr = 0x00aa5500;
	lcd_regs.tcr = 0x0000754f;

    //pxa255_register_state_save(device);
}

static MACHINE_START(39in1)
{
	UINT8 *ROM = memory_region(machine, "maincpu");
	int i;

	for (i = 0; i < 0x80000; i += 2)
	{
		ROM[i] = BITSWAP8(ROM[i],7,2,5,6,0,3,1,4) ^ BITSWAP8((i>>3)&0xf, 3,2,4,1,4,4,0,4) ^ 0x90;
	}

	pxa255_start(machine);
}

static MACHINE_DRIVER_START( 39in1 )
	MDRV_CPU_ADD("maincpu", PXA255, 200000000)
	MDRV_CPU_PROGRAM_MAP(39in1_map)
	MDRV_CPU_VBLANK_INT("screen", pxa255_vblank_start)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(16777216/4, 308, 0,  240, 228, 0,  160)	// completely bogus for this h/w

	MDRV_MACHINE_START(39in1)
	MDRV_NVRAM_HANDLER(93C66B)

	MDRV_VIDEO_UPDATE(39in1)

	MDRV_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
MACHINE_DRIVER_END

ROM_START( 39in1 )
	// main program, encrypted
	ROM_REGION( 0x80000, "maincpu", 0 )
        ROM_LOAD( "27c4096_plz-v001_ver.300.bin", 0x000000, 0x080000, CRC(9149dbc4) SHA1(40efe1f654f11474f75ae7fee1613f435dbede38) )

	// data ROM - contains a filesystem with ROMs, fonts, graphics, etc. in an unknown compressed format
	ROM_REGION32_LE( 0x200000, "data", 0 )
        ROM_LOAD( "16mflash.bin", 0x000000, 0x200000, CRC(a089f0f8) SHA1(e975eadd9176a8b9e416229589dfe3158cba22cb) )
ROM_END

/*ROM_START( arm4in1 )
	// main program, encrypted
	ROM_REGION( 0x80000, "maincpu", 0 )
        ROM_LOAD( "27c4096_plz-v001_ver.300.bin", 0x000000, 0x080000, CRC(9149dbc4) SHA1(40efe1f654f11474f75ae7fee1613f435dbede38) )

	// data ROM - contains a filesystem with ROMs, fonts, graphics, etc. in an unknown compressed format
	ROM_REGION32_LE( 0x200000, "data", 0 )
        ROM_LOAD( "16mflash.bin", 0x000000, 0x200000, CRC(a089f0f8) SHA1(e975eadd9176a8b9e416229589dfe3158cba22cb) )
ROM_END*/

GAME(2004, 39in1, 0, 39in1, 39in1, 0, ROT0, "????", "39 in 1 MAME bootleg", GAME_NOT_WORKING|GAME_NO_SOUND)
//GAME(2004, arm4in1, 0, 39in1, 39in1, 0, ROT0, "????", "4 in 1 MAME bootleg", GAME_NOT_WORKING|GAME_NO_SOUND)
