/**********************************************************************

    ISA cards

    license: MAME, GPL-2.0+
    copyright-holders: Dirk Best

**********************************************************************/

#pragma once

#ifndef __ISA_CARDS_H__
#define __ISA_CARDS_H__

#include "emu.h"

// video
#include "mda.h"
#include "cga.h"
#include "ega.h"
#include "vga.h"
#include "vga_ati.h"
#include "svga_cirrus.h"
#include "svga_s3.h"
#include "svga_tseng.h"

// storage
#include "fdc.h"
#include "mufdc.h"
#include "hdc.h"
#include "wdxt_gen.h"
#include "ide.h"
#include "xtide.h"
#include "side116.h"
#include "aha1542.h"
#include "wd1002a_wx1.h"

// sound
#include "adlib.h"
#include "gblaster.h"
#include "gus.h"
#include "ibm_mfc.h"
#include "mpu401.h"
#include "sblaster.h"
#include "ssi2001.h"
#include "stereo_fx.h"
#include "dectalk.h"

// network
#include "3c503.h"
#include "ne1000.h"
#include "ne2000.h"

// communication ports
#include "lpt.h"
#include "com.h"
#include "pds.h"

// other
#include "finalchs.h"

// supported devices
SLOT_INTERFACE_EXTERN( pc_isa8_cards );
SLOT_INTERFACE_EXTERN( pc_isa16_cards );

#endif // __ISA_CARDS_H__
