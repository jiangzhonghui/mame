class pooyan_state : public driver_device
{
public:
	pooyan_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_colorram(*this, "colorram"),
		m_videoram(*this, "videoram"),
		m_spriteram(*this, "spriteram"),
		m_spriteram2(*this, "spriteram2"),
		m_maincpu(*this, "maincpu"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette") { }

	/* memory pointers */
	required_shared_ptr<UINT8> m_colorram;
	required_shared_ptr<UINT8> m_videoram;
	required_shared_ptr<UINT8> m_spriteram;
	required_shared_ptr<UINT8> m_spriteram2;

	/* video-related */
	tilemap_t  *m_bg_tilemap;

	/* misc */
	UINT8    m_irq_toggle;
	UINT8    m_irq_enable;

	/* devices */
	required_device<cpu_device> m_maincpu;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
	
	DECLARE_WRITE8_MEMBER(irq_enable_w);
	DECLARE_WRITE8_MEMBER(pooyan_videoram_w);
	DECLARE_WRITE8_MEMBER(pooyan_colorram_w);
	DECLARE_WRITE8_MEMBER(pooyan_flipscreen_w);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	DECLARE_PALETTE_INIT(pooyan);
	UINT32 screen_update_pooyan(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(pooyan_interrupt);
	void draw_sprites( bitmap_ind16 &bitmap, const rectangle &cliprect );
};
