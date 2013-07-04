/*************************************************************************

    decocomn.h

**************************************************************************/

#pragma once
#ifndef __DECOCOMN_H__
#define __DECOCOMN_H__


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/


struct decocomn_interface
{
	const char         *m_screen_tag;
};

class decocomn_device : public device_t,
										public decocomn_interface
{
public:
	decocomn_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~decocomn_device() {}
	
	DECLARE_WRITE16_MEMBER( nonbuffered_palette_w );
	DECLARE_WRITE16_MEMBER( buffered_palette_w );
	DECLARE_WRITE16_MEMBER( palette_dma_w );
	DECLARE_WRITE16_MEMBER( priority_w );
	DECLARE_READ16_MEMBER( priority_r );
	DECLARE_READ16_MEMBER( d_71_r );

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_reset();

private:
	// internal state
	screen_device *m_screen;
	UINT8 *m_dirty_palette;
	UINT16 m_priority;
};

extern const device_type DECOCOMN;



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_DECOCOMN_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, DECOCOMN, 0) \
	MCFG_DEVICE_CONFIG(_interface)

#endif
