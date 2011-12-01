/***************************************************************************

	Generic Palette RAMDAC device

***************************************************************************/

#pragma once

#ifndef __ramdacDEV_H__
#define __ramdacDEV_H__

#include "emu.h"


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_RAMDAC_ADD(_tag,_map) \
	MCFG_DEVICE_ADD(_tag, RAMDAC, 0) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _map)


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> v3021_device

class ramdac_device :	public device_t,
						public device_memory_interface
{
public:
	// construction/destruction
	ramdac_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// I/O operations
	DECLARE_READ8_MEMBER( index_r );
	DECLARE_READ8_MEMBER( pal_r );
	DECLARE_WRITE8_MEMBER( index_w );
	DECLARE_WRITE8_MEMBER( pal_w );
	DECLARE_WRITE8_MEMBER( mask_w );

	DECLARE_READ8_MEMBER( ramdac_pal_r );
	DECLARE_WRITE8_MEMBER( ramdac_rgb666_w );
	DECLARE_WRITE8_MEMBER( ramdac_rgb888_w );

	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const;

protected:
	// device-level overrides
	virtual bool device_validity_check(emu_options &options, const game_driver &driver) const;
	virtual void device_start();
	virtual void device_reset();
	inline UINT8 readbyte(offs_t address);
	inline void writebyte(offs_t address, UINT8 data);
	inline void reg_increment();

private:
	UINT8 m_pal_index;
	UINT8 m_pal_mask;
	UINT8 m_int_index;
	UINT8 *m_palram;

	const address_space_config		m_space_config;
};


// device type definition
extern const device_type RAMDAC;



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************



#endif
