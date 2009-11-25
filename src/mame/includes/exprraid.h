/*************************************************************************

    Express Raider

*************************************************************************/


typedef struct _exprraid_state exprraid_state;
struct _exprraid_state
{
	/* memory pointers */
	UINT8 *        main_ram;
	UINT8 *        videoram;
	UINT8 *        colorram;
	UINT8 *        spriteram;

	/* video-related */
	tilemap        *bg_tilemap, *fg_tilemap;
	int            bg_index[4];

	/* misc */
	//int          coin;	// used in the commented out INTERRUPT_GEN - can this be removed?
};


/*----------- defined in video/exprraid.c -----------*/

extern WRITE8_HANDLER( exprraid_videoram_w );
extern WRITE8_HANDLER( exprraid_colorram_w );
extern WRITE8_HANDLER( exprraid_flipscreen_w );
extern WRITE8_HANDLER( exprraid_bgselect_w );
extern WRITE8_HANDLER( exprraid_scrollx_w );
extern WRITE8_HANDLER( exprraid_scrolly_w );

extern VIDEO_START( exprraid );
extern VIDEO_UPDATE( exprraid );
