/*
 * apollo.h - APOLLO DN3500/DN3000 driver includes
 *
 *  Created on: May 12, 2010
 *      Author: Hans Ostermeyer
 *
 *  Released for general non-commercial use under the MAME license
 *  Visit http://mamedev.org for licensing and usage restrictions.
 *
 */

#pragma once

#ifndef APOLLO_H_
#define APOLLO_H_

#include "emu.h"

#include "cpu/m68000/m68000.h"
#include "bus/rs232/rs232.h"
#include "machine/terminal.h"
#include "machine/ram.h"
#include "machine/6840ptm.h"
#include "machine/n68681.h"
#include "machine/am9517a.h"
#include "machine/pic8259.h"
#include "machine/mc146818.h"
#include "machine/apollo_kbd.h"
#include "machine/clock.h"
#include "bus/isa/isa.h"
#include "bus/isa/isa_cards.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#define LOG(x)  { logerror x; logerror ("\n"); apollo_check_log(); }
#define LOG1(x) { if (VERBOSE > 0) LOG(x) }
#define LOG2(x) { if (VERBOSE > 1) LOG(x) }
#define CLOG(x) { logerror ("%s - %s: ", apollo_cpu_context(machine().device(MAINCPU)), tag()); LOG(x) }
#define CLOG1(x) { if (VERBOSE > 0) CLOG(x) }
#define CLOG2(x) { if (VERBOSE > 1) CLOG(x) }
#define DLOG(x) { logerror ("%s - %s: ", apollo_cpu_context(device->machine().device(MAINCPU)), device->tag()); LOG(x) }
#define DLOG1(x) { if (VERBOSE > 0) DLOG(x) }
#define DLOG2(x) { if (VERBOSE > 1) DLOG(x) }
#define MLOG(x)  { logerror ("%s: ", apollo_cpu_context(machine().device(MAINCPU))); LOG(x) }
#define MLOG1(x) { if (VERBOSE > 0) MLOG(x) }
#define MLOG2(x) { if (VERBOSE > 1) MLOG(x) }
#define SLOG(x)  { logerror ("%s: ", apollo_cpu_context(&space.device())); LOG(x) }
#define SLOG1(x) { if (VERBOSE > 0) SLOG(x) }
#define SLOG2(x) { if (VERBOSE > 1) SLOG(x) }

#define  MAINCPU "maincpu"

/*----------- machine/apollo_dbg.c -----------*/
int apollo_debug_instruction_hook(m68000_base_device *device, offs_t curpc);

/*----------- drivers/apollo.c -----------*/

// return the current CPU context for log file entries
const char *apollo_cpu_context(device_t *cpu);

// enable/disable the FPU
void apollo_set_cpu_has_fpu(m68000_base_device *device, int onoff);

// check for excessive logging
void apollo_check_log();

// return 1 if node is DN3000 or DSP3000, 0 otherwise
int apollo_is_dn3000(void);

// return 1 if node is DSP3000 or DSP3500, 0 otherwise
int apollo_is_dsp3x00(void);

// get the ram configuration byte
UINT8 apollo_get_ram_config_byte(void);

//apollo_get_node_id - get the node id
UINT32 apollo_get_node_id(void);

	// should be called by the CPU core before executing each instruction
int apollo_instruction_hook(m68000_base_device *device, offs_t curpc);

void apollo_set_cache_status_register(UINT8 mask, UINT8 data);

/*----------- machine/apollo.c -----------*/

#define APOLLO_CONF_TAG "conf"
#define APOLLO_DMA1_TAG "dma8237_1"
#define APOLLO_DMA2_TAG "dma8237_2"
#define APOLLO_KBD_TAG  "kbd"
#define APOLLO_PIC1_TAG "pic8259_master"
#define APOLLO_PIC2_TAG "pic8259_slave"
#define APOLLO_PTM_TAG  "ptm"
#define APOLLO_RTC_TAG  "rtc"
#define APOLLO_SIO_TAG  "sio"
#define APOLLO_SIO2_TAG "sio2"
#define APOLLO_ETH_TAG  "3c505"
#define APOLLO_ISA_TAG "isabus"

class apollo_state : public driver_device
{
public:
	apollo_state(const machine_config &mconfig, device_type type, const char *tag)
			: driver_device(mconfig, type, tag),
			m_maincpu(*this, MAINCPU),
			m_messram_ptr(*this, "messram"),
			m_dma8237_1(*this, APOLLO_DMA1_TAG),
			m_dma8237_2(*this, APOLLO_DMA2_TAG),
			m_pic8259_master(*this, APOLLO_PIC1_TAG),
			m_pic8259_slave(*this, APOLLO_PIC2_TAG),
			m_ptm(*this, APOLLO_PTM_TAG),
			m_sio(*this, APOLLO_SIO_TAG),
			m_sio2(*this, APOLLO_SIO2_TAG),
			m_rtc(*this, APOLLO_RTC_TAG),
			m_isa(*this, APOLLO_ISA_TAG)
			{ }

	required_device<m68000_base_device> m_maincpu;
	required_shared_ptr<UINT32> m_messram_ptr;

	required_device<am9517a_device> m_dma8237_1;
	required_device<am9517a_device> m_dma8237_2;
	required_device<pic8259_device> m_pic8259_master;
	required_device<pic8259_device> m_pic8259_slave;
	required_device<ptm6840_device> m_ptm;
	required_device<duartn68681_device> m_sio;
	optional_device<duartn68681_device> m_sio2;
	required_device<mc146818_device> m_rtc;
	required_device<isa16_device> m_isa;

	DECLARE_WRITE16_MEMBER(apollo_csr_status_register_w);
	DECLARE_READ16_MEMBER(apollo_csr_status_register_r);
	DECLARE_WRITE16_MEMBER(apollo_csr_control_register_w);
	DECLARE_READ16_MEMBER(apollo_csr_control_register_r);
	DECLARE_WRITE8_MEMBER(apollo_dma_1_w);
	DECLARE_READ8_MEMBER(apollo_dma_1_r);
	DECLARE_WRITE8_MEMBER(apollo_dma_2_w);
	DECLARE_READ8_MEMBER(apollo_dma_2_r);
	DECLARE_WRITE8_MEMBER(apollo_dma_page_register_w);
	DECLARE_READ8_MEMBER(apollo_dma_page_register_r);
	DECLARE_WRITE16_MEMBER(apollo_address_translation_map_w);
	DECLARE_READ16_MEMBER(apollo_address_translation_map_r);
	DECLARE_READ8_MEMBER(apollo_dma_read_byte);
	DECLARE_WRITE8_MEMBER(apollo_dma_write_byte);
	DECLARE_READ8_MEMBER(apollo_dma_read_word);
	DECLARE_WRITE8_MEMBER(apollo_dma_write_word);
	DECLARE_WRITE8_MEMBER(apollo_rtc_w);
	DECLARE_READ8_MEMBER(apollo_rtc_r);
	DECLARE_WRITE8_MEMBER(cache_control_register_w);
	DECLARE_READ8_MEMBER(cache_status_register_r);
	DECLARE_WRITE8_MEMBER(task_alias_register_w);
	DECLARE_READ8_MEMBER(task_alias_register_r);
	DECLARE_WRITE16_MEMBER(apollo_node_id_w);
	DECLARE_READ16_MEMBER(apollo_node_id_r);
	DECLARE_WRITE16_MEMBER(latch_page_on_parity_error_register_w);
	DECLARE_READ16_MEMBER(latch_page_on_parity_error_register_r);
	DECLARE_WRITE8_MEMBER(master_req_register_w);
	DECLARE_READ8_MEMBER(master_req_register_r);
	DECLARE_WRITE16_MEMBER(selective_clear_locations_w);
	DECLARE_READ16_MEMBER(selective_clear_locations_r);
	DECLARE_READ32_MEMBER(ram_with_parity_r);
	DECLARE_WRITE32_MEMBER(ram_with_parity_w);
	DECLARE_READ32_MEMBER(apollo_unmapped_r);
	DECLARE_WRITE32_MEMBER(apollo_unmapped_w);
	DECLARE_WRITE32_MEMBER(apollo_rom_w);
	DECLARE_READ16_MEMBER(apollo_atbus_io_r);
	DECLARE_WRITE16_MEMBER(apollo_atbus_io_w);
	DECLARE_READ16_MEMBER(apollo_atbus_memory_r);
	DECLARE_WRITE16_MEMBER(apollo_atbus_memory_w);
	DECLARE_WRITE8_MEMBER(dn5500_memory_present_register_w);
	DECLARE_READ8_MEMBER(dn5500_memory_present_register_r);
	DECLARE_WRITE8_MEMBER(dn5500_11500_w);
	DECLARE_READ8_MEMBER(dn5500_11500_r);
	DECLARE_WRITE8_MEMBER(dn5500_io_protection_map_w);
	DECLARE_READ8_MEMBER(dn5500_io_protection_map_r);
	DECLARE_READ32_MEMBER(apollo_f8_r);
	DECLARE_WRITE32_MEMBER(apollo_f8_w);
	DECLARE_DRIVER_INIT(dsp3000);
	DECLARE_DRIVER_INIT(dsp5500);
	DECLARE_DRIVER_INIT(dn3500);
	DECLARE_DRIVER_INIT(dn3000);
	DECLARE_DRIVER_INIT(dsp3500);
	DECLARE_DRIVER_INIT(dn5500);
	DECLARE_DRIVER_INIT(apollo);

	virtual void machine_start();
	virtual void machine_reset();
	DECLARE_MACHINE_RESET(apollo);
	DECLARE_MACHINE_START(apollo);

	IRQ_CALLBACK_MEMBER(apollo_irq_acknowledge);
	IRQ_CALLBACK_MEMBER(apollo_pic_acknowledge);
	void apollo_bus_error();
	DECLARE_WRITE8_MEMBER( apollo_kbd_putchar );
	DECLARE_READ_LINE_MEMBER( apollo_kbd_is_german );
	DECLARE_WRITE_LINE_MEMBER( apollo_dma8237_out_eop );
	DECLARE_WRITE_LINE_MEMBER( apollo_dma_1_hrq_changed );
	DECLARE_WRITE_LINE_MEMBER( apollo_dma_2_hrq_changed );
	DECLARE_WRITE_LINE_MEMBER( apollo_pic8259_master_set_int_line );
	DECLARE_WRITE_LINE_MEMBER( apollo_pic8259_slave_set_int_line );
	DECLARE_WRITE_LINE_MEMBER( sio_irq_handler );
	DECLARE_WRITE8_MEMBER( sio_output );
	DECLARE_WRITE_LINE_MEMBER( sio2_irq_handler );
	DECLARE_WRITE_LINE_MEMBER( apollo_ptm_irq_function );
	DECLARE_WRITE_LINE_MEMBER( apollo_ptm_timer_tick );
	DECLARE_READ8_MEMBER( apollo_pic8259_get_slave_ack );

	DECLARE_READ8_MEMBER(pc_dma8237_0_dack_r);
	DECLARE_READ8_MEMBER(pc_dma8237_1_dack_r);
	DECLARE_READ8_MEMBER(pc_dma8237_2_dack_r);
	DECLARE_READ8_MEMBER(pc_dma8237_3_dack_r);
	DECLARE_READ8_MEMBER(pc_dma8237_5_dack_r);
	DECLARE_READ8_MEMBER(pc_dma8237_6_dack_r);
	DECLARE_READ8_MEMBER(pc_dma8237_7_dack_r);
	DECLARE_WRITE8_MEMBER(pc_dma8237_0_dack_w);
	DECLARE_WRITE8_MEMBER(pc_dma8237_1_dack_w);
	DECLARE_WRITE8_MEMBER(pc_dma8237_2_dack_w);
	DECLARE_WRITE8_MEMBER(pc_dma8237_3_dack_w);
	DECLARE_WRITE8_MEMBER(pc_dma8237_5_dack_w);
	DECLARE_WRITE8_MEMBER(pc_dma8237_6_dack_w);
	DECLARE_WRITE8_MEMBER(pc_dma8237_7_dack_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack0_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack1_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack2_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack3_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack4_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack5_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack6_w);
	DECLARE_WRITE_LINE_MEMBER(pc_dack7_w);
	TIMER_CALLBACK_MEMBER( apollo_rtc_timer );

	void apollo_pic_set_irq_line(int irq, int state);
	void select_dma_channel(int channel, bool state);

private:
	UINT32 ptm_counter;
	UINT8 sio_output_data;
	int m_dma_channel;
	bool m_cur_eop;
	emu_timer *m_dn3000_timer;
};

MACHINE_CONFIG_EXTERN( apollo );
MACHINE_CONFIG_EXTERN( apollo_terminal );

/*----------- machine/apollo_config.c -----------*/

// configuration bit definitions

#define APOLLO_CONF_SERVICE_MODE 0x0001
#define APOLLO_CONF_DISPLAY      0x001e
#define APOLLO_CONF_8_PLANES     0x0002
#define APOLLO_CONF_4_PLANES     0x0004
#define APOLLO_CONF_MONO_15I     0x0008
#define APOLLO_CONF_MONO_19I     0x0010
#define APOLLO_CONF_GERMAN_KBD   0x0020
#define APOLLO_CONF_DATE_1990    0x0040
#define APOLLO_CONF_NODE_ID      0x0080
#define APOLLO_CONF_IDLE_SLEEP   0x0100
#define APOLLO_CONF_TRAP_TRACE   0x0200
#define APOLLO_CONF_FPU_TRACE    0x0400
#define APOLLO_CONF_DISK_TRACE   0x0800
#define APOLLO_CONF_NET_TRACE    0x1000

// check configuration setting
int apollo_config(int mask);

INPUT_PORTS_EXTERN(apollo_config);

/*----------- machine/apollo_csr.c -----------*/

#define APOLLO_CSR_SR_SERVICE            0x0001
#define APOLLO_CSR_SR_ATBUS_IO_TIMEOUT   0x0002
#define APOLLO_CSR_SR_FP_TRAP            0x0004
#define APOLLO_CSR_SR_INTERRUPT_PENDING  0x0008 // DN3000 only
#define APOLLO_CSR_SR_PARITY_BYTE_MASK   0x00f0
#define APOLLO_CSR_SR_CPU_TIMEOUT        0x0100
#define APOLLO_CSR_SR_ATBUS_MEM_TIMEOUT  0x2000
#define APOLLO_CSR_SR_BIT15              0x8000
#define APOLLO_CSR_SR_CLEAR_ALL          0x3ffe

#define APOLLO_CSR_CR_INTERRUPT_ENABLE   0x0001
#define APOLLO_CSR_CR_RESET_DEVICES       0x0002
#define APOLLO_CSR_CR_FPU_TRAP_ENABLE    0x0004
#define APOLLO_CSR_CR_FORCE_BAD_PARITY   0x0008
#define APOLLO_CSR_CR_PARITY_BYTE_MASK   0x00f0

UINT16 apollo_csr_get_control_register(void);
UINT16 apollo_csr_get_status_register(void);
void apollo_csr_set_status_register(UINT16 mask, UINT16 data);

/*----------- video/apollo.c -----------*/

#define APOLLO_SCREEN_TAG "apollo_screen"

class apollo_graphics_15i : public device_t
{
public:
	apollo_graphics_15i(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	apollo_graphics_15i(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock, device_type type, const char *name, const char *shortname, const char *source);
	~apollo_graphics_15i();

	// access to legacy token
	class apollo_graphics *token() const { assert(m_token != NULL); return m_token; }
protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();
private:
	// internal state
	class apollo_graphics *m_token;
};

extern const device_type APOLLO_GRAPHICS;

#define MCFG_APOLLO_GRAPHICS_ADD( _tag) \
	MCFG_FRAGMENT_ADD(apollo_graphics) \
	MCFG_DEVICE_ADD(_tag, APOLLO_GRAPHICS, 0)

MACHINE_CONFIG_EXTERN( apollo_graphics );

class apollo_graphics_19i : public apollo_graphics_15i
{
public:
	apollo_graphics_19i(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();
private:
	// internal state
};

extern const device_type APOLLO_MONO19I;

#define MCFG_APOLLO_MONO19I_ADD(_tag) \
	MCFG_FRAGMENT_ADD(apollo_mono19i) \
	MCFG_DEVICE_ADD(_tag, APOLLO_MONO19I, 0)

MACHINE_CONFIG_EXTERN( apollo_mono19i );

DECLARE_READ8_DEVICE_HANDLER( apollo_mcr_r ) ;
DECLARE_WRITE8_DEVICE_HANDLER(apollo_mcr_w );

DECLARE_READ16_DEVICE_HANDLER( apollo_mgm_r );
DECLARE_WRITE16_DEVICE_HANDLER( apollo_mgm_w );

DECLARE_READ8_DEVICE_HANDLER( apollo_ccr_r ) ;
DECLARE_WRITE8_DEVICE_HANDLER(apollo_ccr_w );

DECLARE_READ16_DEVICE_HANDLER( apollo_cgm_r );
DECLARE_WRITE16_DEVICE_HANDLER( apollo_cgm_w );

#endif /* APOLLO_H_ */
