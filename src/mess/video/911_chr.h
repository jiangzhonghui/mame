/*
    911_chr.h: character definitions for 911_vdt.c

    We define the character matrix for each character.

    The US terminal uses the standard 7-bit ASCII character set, with
    additional graphic characters in the 32 first positions.

    The various European terminals use variants of the 7-bit ASCII character
    set with national characters instead of various punctuation characters.
    I think these national character sets were standardized at a point, but
    I don't know how close to this standard the character sets used by the
    911 VDT are.

    The japanese terminal uses 8-bit character codes.  The 128 first characters
    are identical to the US character set (except that '\' is replaced by the
    Yen symbol), and the next 128 characters include the katakana syllabus.
    Kanji ideograms are not supported in this scheme.

    The arabic terminal uses 8-bit character codes, too.  It requires
    additional code in the TI990 OS for correct operation, as the keyboard
    returns codes for isolated characters (i.e. without ligatures), which need
    to be substituted with codes with correct context-dependent ligatures.
    And both OS and application programs need to support the fact that the
    writing direction can be either right-to-left or left-to-right, according
    to whether the characters are latin or arabic.

    As the original ROMs have not been dumped yet, I recreated the matrices
    from various matrix printouts in TI documentation.
*/
/*
    The arabic character set is not implemented, because documentation is ambiguous
    (it says there are 115 characters, but I can hardly see 80 characters in the
    attached table), and the character matrices are not documented.
*/


/*
    Offsets in the char_defs array
*/
enum
{
	/* US ASCII: 128 characters (32 symbols + 95 ASCII + 1 blank (delete character)) */
	char_defs_US_base = 0,
	/* additionnal katakana set (128 characters, including JIS set) */
	char_defs_katakana_base = char_defs_US_base+128,
	/* extra symbols for national character sets */
	char_defs_pound = char_defs_katakana_base+128,	/* pound sign (UK 0x23, French WP 0x23) */
	char_defs_yen,		/* yen sign (Japan 0x5C) */
	char_defs_auml,		/* latin small letter a with diaeresis (Swedish/Finish 0x7B, German 0x7B) */
	char_defs_Auml,		/* latin capital letter A with diaeresis (Swedish/Finish 0x5B, German 0x5B) */
	char_defs_Aring,	/* latin capital letter A with ring above (Swedish/Finish 0x5D, Norwegian/Danish 0x5D) */
	char_defs_uuml,		/* latin small letter u with diaeresis (Swedish/Finish 0x7E, German 0x7D) */
	char_defs_aring,	/* latin small letter a with ring above (Swedish/Finish 0x7D, Norwegian/Danish 0x7D) */
	char_defs_Uuml,		/* latin capital letter U with diaeresis (German 0x5D) */
	char_defs_ouml,		/* latin small letter o with diaeresis (German 0x7C) */
	char_defs_Ouml,		/* latin capital letter O with diaeresis (German 0x5C) */
	char_defs_szlig,	/* latin small letter sharp s (German 0x7E) */
	char_defs_aelig,	/* latin small letter ae (Norwegian/Danish 0x7B) */
	char_defs_AElig,	/* latin capital letter AE (Norwegian/Danish 0x5B) */
	char_defs_oslash,	/* latin small letter o with stroke (Norwegian/Danish 0x7C) */
	char_defs_Oslash,	/* latin capital letter O with stroke (Norwegian/Danish 0x5C) */
	char_defs_agrave,	/* latin small letter a with grave (French WP 0x40) */
	char_defs_deg,		/* degree sign (French WP 0x5B) */
	char_defs_ccedil,	/* latin small letter c with cedilla (French WP 0x5C) */
	char_defs_sect,		/* section sign (French WP 0x5D) */
	char_defs_egrave,	/* latin small letter e with grave (French WP 0x7B) */
	char_defs_ugrave,	/* latin small letter u with grave (French WP 0x7C) */
	char_defs_eacute,	/* latin small letter e with acute (French WP 0x7D) */
	char_defs_uml,		/* diaeresis (French WP 0x7E) */

	char_defs_count		/* total character count */
};

/* structure used to describe differences between national character sets and
US character set */
/* much more compact than defining the complete 128-char vector */
typedef struct char_override_t
{
	unsigned char char_index;		/* char to replace */
	unsigned short symbol_index;	/* replacement symbol */
} char_override_t;

/* One UK-specific character */
static const char_override_t UK_overrides[1] =
{
	{	0x23,	char_defs_pound	}
};

/* One japan-specific character (see below for the 128 additionnal characters) */
static const char_override_t japanese_overrides[1] =
{
	{	0x5C,	char_defs_yen	}
};

/* 5 sweden/finland-specific characters */
static const char_override_t swedish_overrides[/*5*/7] =
{
	{	0x7B,	char_defs_auml	},
	{	0x5B,	char_defs_Auml	},
	{	0x5D,	char_defs_Aring	},
	{	0x7E,	char_defs_uuml	},
	{	0x7D,	char_defs_aring	},
	/* next characters described in D-4 but not 1-10 */
	{	0x5C,	char_defs_Ouml	},
	{	0x7C,	char_defs_ouml	}
};

/* 7 german-specific characters */
static const char_override_t german_overrides[7] =
{
	{	0x5D,	char_defs_Uuml	},
	{	0x7D,	char_defs_uuml	},
	{	0x7C,	char_defs_ouml	},
	{	0x5C,	char_defs_Ouml	},
	{	0x7B,	char_defs_auml	},
	{	0x7E,	char_defs_szlig	},
	{	0x5B,	char_defs_Auml	}	/* 945423-9701 rev. B p. 1-10 says 0x5D, but it must be a mistake */
};

/* 6 norway/denmark-specific characters */
static const char_override_t norwegian_overrides[6] =
{
	{	0x5D,	char_defs_Aring	},
	{	0x7B,	char_defs_aelig	},
	{	0x5B,	char_defs_AElig	},
	{	0x7D,	char_defs_aring	},
	{	0x7C,	char_defs_oslash},
	{	0x5C,	char_defs_Oslash}
};

/* 9 french-specific characters (word-processing model only: the data-processing model uses
the US character set, although the keyboard mapping is different from the US model) */
/* WARNING: I have created the character matrices from scratch, as I have no printout of
the original matrices. */
static const char_override_t frenchWP_overrides[9] =
{
	{	0x23,	char_defs_pound	},
	{	0x40,	char_defs_agrave},
	{	0x5B,	char_defs_deg	},
	{	0x5C,	char_defs_ccedil},
	{	0x5D,	char_defs_sect	},
	{	0x7B,	char_defs_eacute},	/* 945423-9701 rev. B says char_defs_egrave, but it must be a mistake */
	{	0x7C,	char_defs_ugrave},
	{	0x7D,	char_defs_egrave},	/* 945423-9701 rev. B says char_defs_eacute, but it must be a mistake */
	{	0x7E,	char_defs_uml	}
};

/*
    char_defs array: character matrices for each character
*/
static const UINT8 char_defs[char_defs_count][10] =
{

/* US character set: 128 7*10 character matrix */
	{	/* 0x00 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c
	},
	{	/* 0x01 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c
	},
	{	/* 0x02 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x03 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x04 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x05 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x06 */
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x07 */
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x08 */
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x09 */
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x0A */
		0x00,
		0x00,
		0x00,
		0x1f,
		0x1f,
		0x1f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x0B */
		0x1c,
		0x1c,
		0x1c,
		0x7c,
		0x7c,
		0x7c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x0C */
		0x00,
		0x00,
		0x00,
		0x7f,
		0x7f,
		0x7f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x0D */
		0x1c,
		0x1c,
		0x1c,
		0x1f,
		0x1f,
		0x1f,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x0E */
		0x01,
		0x03,
		0x06,
		0x04,
		0x08,
		0x08,
		0x10,
		0x30,
		0x60,
		0x40
	},
	{	/* 0x0F */
		0x00,
		0x00,
		0x7f,
		0x00,
		0x00,
		0x00,
		0x7f,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x10 */
		0x00,
		0x00,
		0x00,
		0x40,
		0x40,
		0x40,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x11 */
		0x00,
		0x00,
		0x00,
		0x60,
		0x60,
		0x60,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x12 */
		0x00,
		0x00,
		0x00,
		0x70,
		0x70,
		0x70,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x13 */
		0x00,
		0x00,
		0x00,
		0x78,
		0x78,
		0x78,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x14*/
		0x00,
		0x00,
		0x00,
		0x7c,
		0x7c,
		0x7c,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x15 */
		0x00,
		0x00,
		0x00,
		0x7e,
		0x7e,
		0x7e,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x16 */
		0x00,
		0x00,
		0x00,
		0x7f,
		0x7f,
		0x7f,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x17*/
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x18 */
		0x1c,
		0x1c,
		0x1c,
		0x7f,
		0x7f,
		0x7f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x19 */
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f
	},
	{	/* 0x1A */
		0x00,
		0x00,
		0x00,
		0x7c,
		0x7c,
		0x7c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x1B */
		0x1c,
		0x1c,
		0x1c,
		0x1f,
		0x1f,
		0x1f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x1C */
		0x1c,
		0x1c,
		0x1c,
		0x7f,
		0x7f,
		0x7f,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x1D */
		0x1c,
		0x1c,
		0x1c,
		0x7c,
		0x7c,
		0x7c,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x1E */
		0x40,
		0x60,
		0x30,
		0x10,
		0x08,
		0x08,
		0x04,
		0x06,
		0x03,
		0x01
	},
	{	/* 0x1F */
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22
	},
	{	/* 0x20 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x21 */
		0x00,
		0x18,
		0x18,
		0x18,
		0x18,
		0x18,
		0x00,
		0x18,
		0x00,
		0x00
	},
	{	/* 0x22 */
		0x00,
		0x14,
		0x14,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x23 */
		0x00,
		0x00,
		0x14,
		0x36,
		0x00,
		0x36,
		0x14,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x24 */
		0x00,
		0x1c,
		0x2a,
		0x28,
		0x1c,
		0x0a,
		0x2a,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x25 */
		0x00,
		0x30,
		0x32,
		0x04,
		0x08,
		0x10,
		0x26,
		0x06,
		0x00,
		0x00
	},
	{	/* 0x26 */
		0x00,
		0x10,
		0x28,
		0x28,
		0x10,
		0x2a,
		0x24,
		0x1a,
		0x00,
		0x00
	},
	{	/* 0x27 */
		0x00,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x28 */
		0x00,
		0x02,
		0x04,
		0x08,
		0x08,
		0x08,
		0x04,
		0x02,
		0x00,
		0x00
	},
	{	/* 0x29 */
		0x00,
		0x20,
		0x10,
		0x08,
		0x08,
		0x08,
		0x10,
		0x20,
		0x00,
		0x00
	},
	{	/* 0x2A */
		0x00,
		0x00,
		0x08,
		0x1c,
		0x3e,
		0x1c,
		0x08,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x2B */
		0x00,
		0x00,
		0x08,
		0x08,
		0x3e,
		0x08,
		0x08,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x2C */
		0x00,
		0x00,
		0x00,
		0x00,
		0x18,
		0x18,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0x2D */
		0x00,
		0x00,
		0x00,
		0x00,
		0x3e,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x2E */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x18,
		0x18,
		0x00,
		0x00
	},
	{	/* 0x2F */
		0x00,
		0x00,
		0x02,
		0x04,
		0x08,
		0x10,
		0x20,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x30 */
		0x00,
		0x0c,
		0x12,
		0x12,
		0x12,
		0x12,
		0x12,
		0x0c,
		0x00,
		0x00
	},
	{	/* 0x31 */
		0x00,
		0x08,
		0x18,
		0x08,
		0x08,
		0x08,
		0x08,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x32 */
		0x00,
		0x1c,
		0x22,
		0x02,
		0x1c,
		0x20,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x33 */
		0x00,
		0x1c,
		0x22,
		0x02,
		0x0c,
		0x02,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x34 */
		0x00,
		0x04,
		0x0c,
		0x14,
		0x24,
		0x3e,
		0x04,
		0x04,
		0x00,
		0x00
	},
	{	/* 0x35 */
		0x00,
		0x3e,
		0x20,
		0x3c,
		0x02,
		0x02,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x36 */
		0x00,
		0x0c,
		0x10,
		0x20,
		0x3c,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x37 */
		0x00,
		0x3e,
		0x02,
		0x04,
		0x08,
		0x10,
		0x10,
		0x10,
		0x00,
		0x00
	},
	{	/* 0x38 */
		0x00,
		0x1c,
		0x22,
		0x22,
		0x1c,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x39 */
		0x00,
		0x1c,
		0x22,
		0x22,
		0x1e,
		0x02,
		0x04,
		0x18,
		0x00,
		0x00
	},
	{	/* 0x3A */
		0x00,
		0x00,
		0x18,
		0x18,
		0x00,
		0x18,
		0x18,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x3B */
		0x00,
		0x18,
		0x18,
		0x00,
		0x18,
		0x18,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0x3C */
		0x00,
		0x02,
		0x04,
		0x08,
		0x10,
		0x08,
		0x04,
		0x02,
		0x00,
		0x00
	},
	{	/* 0x3D */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x00,
		0x3e,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x3E */
		0x00,
		0x20,
		0x10,
		0x08,
		0x04,
		0x08,
		0x10,
		0x20,
		0x00,
		0x00
	},
	{	/* 0x3F */
		0x00,
		0x1c,
		0x22,
		0x04,
		0x08,
		0x08,
		0x00,
		0x08,
		0x00,
		0x00
	},
	{	/* 0x40 */
		0x00,
		0x1c,
		0x22,
		0x2e,
		0x2a,
		0x2e,
		0x20,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x41 */
		0x00,
		0x1c,
		0x22,
		0x22,
		0x3e,
		0x22,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x42 */
		0x00,
		0x3c,
		0x12,
		0x12,
		0x1c,
		0x12,
		0x12,
		0x3c,
		0x00,
		0x00
	},
	{	/* 0x43 */
		0x00,
		0x1c,
		0x22,
		0x20,
		0x20,
		0x20,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x44 */
		0x00,
		0x3c,
		0x12,
		0x12,
		0x12,
		0x12,
		0x12,
		0x3c,
		0x00,
		0x00
	},
	{	/* 0x45 */
		0x00,
		0x3e,
		0x20,
		0x20,
		0x3c,
		0x20,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x46 */
		0x00,
		0x3e,
		0x20,
		0x20,
		0x3c,
		0x20,
		0x20,
		0x20,
		0x00,
		0x00
	},
	{	/* 0x47 */
		0x00,
		0x1e,
		0x20,
		0x20,
		0x2e,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x48 */
		0x00,
		0x22,
		0x22,
		0x22,
		0x3e,
		0x22,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x49 */
		0x00,
		0x1c,
		0x08,
		0x08,
		0x08,
		0x08,
		0x08,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x4A */
		0x00,
		0x02,
		0x02,
		0x02,
		0x02,
		0x02,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x4B */
		0x00,
		0x22,
		0x24,
		0x28,
		0x30,
		0x28,
		0x24,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x4C */
		0x00,
		0x20,
		0x20,
		0x20,
		0x20,
		0x20,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x4D */
		0x00,
		0x22,
		0x36,
		0x2a,
		0x22,
		0x22,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x4E */
		0x00,
		0x22,
		0x32,
		0x2a,
		0x26,
		0x22,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x4F */
		0x00,
		0x3e,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x50 */
		0x00,
		0x3c,
		0x22,
		0x22,
		0x3c,
		0x20,
		0x20,
		0x20,
		0x00,
		0x00
	},
	{	/* 0x51 */
		0x00,
		0x3e,
		0x22,
		0x22,
		0x22,
		0x2a,
		0x24,
		0x3a,
		0x00,
		0x00
	},
	{	/* 0x52 */
		0x00,
		0x3c,
		0x22,
		0x22,
		0x3c,
		0x28,
		0x24,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x53 */
		0x00,
		0x1c,
		0x20,
		0x10,
		0x08,
		0x04,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x54 */
		0x00,
		0x3e,
		0x08,
		0x08,
		0x08,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0x55 */
		0x00,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x56 */
		0x00,
		0x22,
		0x22,
		0x22,
		0x14,
		0x14,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0x57 */
		0x00,
		0x22,
		0x22,
		0x22,
		0x2a,
		0x2a,
		0x2a,
		0x14,
		0x00,
		0x00
	},
	{	/* 0x58 */
		0x00,
		0x22,
		0x22,
		0x14,
		0x08,
		0x14,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x59 */
		0x00,
		0x22,
		0x22,
		0x14,
		0x08,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0x5A */
		0x00,
		0x3e,
		0x02,
		0x04,
		0x08,
		0x10,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x5B */
		0x00,
		0x0e,
		0x08,
		0x08,
		0x08,
		0x08,
		0x08,
		0x0e,
		0x00,
		0x00
	},
	{	/* 0x5C */
		0x00,
		0x00,
		0x20,
		0x10,
		0x08,
		0x04,
		0x02,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x5D */
		0x00,
		0x38,
		0x08,
		0x08,
		0x08,
		0x08,
		0x08,
		0x38,
		0x00,
		0x00
	},
	{	/* 0x5E */
		0x00,
		0x08,
		0x14,
		0x22,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x5F */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x60 */
		0x00,
		0x10,
		0x08,
		0x04,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x61 */
		0x00,
		0x00,
		0x00,
		0x1c,
		0x22,
		0x3e,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x62 */
		0x00,
		0x00,
		0x00,
		0x3c,
		0x12,
		0x1c,
		0x12,
		0x3c,
		0x00,
		0x00
	},
	{	/* 0x63 */
		0x00,
		0x00,
		0x00,
		0x1e,
		0x20,
		0x20,
		0x20,
		0x1e,
		0x00,
		0x00
	},
	{	/* 0x64 */
		0x00,
		0x00,
		0x00,
		0x3c,
		0x12,
		0x12,
		0x12,
		0x3c,
		0x00,
		0x00
	},
	{	/* 0x65 */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x20,
		0x3c,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x66 */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x20,
		0x3c,
		0x20,
		0x20,
		0x00,
		0x00
	},
	{	/* 0x67 */
		0x00,
		0x00,
		0x00,
		0x1e,
		0x20,
		0x2e,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x68 */
		0x00,
		0x00,
		0x00,
		0x22,
		0x22,
		0x3e,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x69 */
		0x00,
		0x00,
		0x00,
		0x1c,
		0x08,
		0x08,
		0x08,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x6A */
		0x00,
		0x00,
		0x00,
		0x0e,
		0x04,
		0x04,
		0x24,
		0x3c,
		0x00,
		0x00
	},
	{	/* 0x6B */
		0x00,
		0x00,
		0x00,
		0x22,
		0x24,
		0x38,
		0x24,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x6C */
		0x00,
		0x00,
		0x00,
		0x20,
		0x20,
		0x20,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x6D */
		0x00,
		0x00,
		0x00,
		0x22,
		0x36,
		0x2a,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x6E */
		0x00,
		0x00,
		0x00,
		0x22,
		0x32,
		0x2a,
		0x26,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x6F */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x22,
		0x22,
		0x22,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x70 */
		0x00,
		0x00,
		0x00,
		0x3c,
		0x22,
		0x3c,
		0x20,
		0x20,
		0x00,
		0x00
	},
	{	/* 0x71 */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x22,
		0x2a,
		0x24,
		0x3a,
		0x00,
		0x00
	},
	{	/* 0x72 */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x22,
		0x3e,
		0x24,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x73 */
		0x00,
		0x00,
		0x00,
		0x1e,
		0x20,
		0x1c,
		0x02,
		0x3c,
		0x00,
		0x00
	},
	{	/* 0x74 */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x08,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0x75 */
		0x00,
		0x00,
		0x00,
		0x22,
		0x22,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0x76 */
		0x00,
		0x00,
		0x00,
		0x22,
		0x22,
		0x24,
		0x28,
		0x10,
		0x00,
		0x00
	},
	{	/* 0x77 */
		0x00,
		0x00,
		0x00,
		0x22,
		0x22,
		0x2a,
		0x36,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x78 */
		0x00,
		0x00,
		0x00,
		0x22,
		0x14,
		0x08,
		0x14,
		0x22,
		0x00,
		0x00
	},
	{	/* 0x79 */
		0x00,
		0x00,
		0x00,
		0x22,
		0x14,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0x7A */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x04,
		0x08,
		0x10,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0x7B */
		0x00,
		0x06,
		0x08,
		0x08,
		0x10,
		0x08,
		0x08,
		0x06,
		0x00,
		0x00
	},
	{	/* 0x7C */
		0x00,
		0x08,
		0x08,
		0x08,
		0x00,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0x7D */
		0x00,
		0x30,
		0x08,
		0x08,
		0x04,
		0x08,
		0x08,
		0x30,
		0x00,
		0x00
	},
	{	/* 0x7E */
		0x00,
		0x10,
		0x2a,
		0x04,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x7F */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},


/* 128 additional characters for japanese terminals */
	{	/* 0x80 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c
	},
	{	/* 0x81 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c
	},
	{	/* 0x82 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x83 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x84 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x85 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x86 */
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x87 */
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x88 */
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x89 */
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x8A */
		0x00,
		0x00,
		0x00,
		0x1f,
		0x1f,
		0x1f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x8B */
		0x1c,
		0x1c,
		0x1c,
		0x7c,
		0x7c,
		0x7c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x8C */
		0x00,
		0x00,
		0x00,
		0x7f,
		0x7f,
		0x7f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x8D */
		0x1c,
		0x1c,
		0x1c,
		0x1f,
		0x1f,
		0x1f,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x8E */
		0x01,
		0x03,
		0x06,
		0x04,
		0x08,
		0x08,
		0x10,
		0x30,
		0x30,
		0x20
	},
	{	/* 0x8F */
		0x00,
		0x00,
		0x7f,
		0x00,
		0x00,
		0x00,
		0x7f,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x90 */
		0x00,
		0x00,
		0x00,
		0x40,
		0x40,
		0x40,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x91 */
		0x00,
		0x00,
		0x00,
		0x60,
		0x60,
		0x60,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x92 */
		0x00,
		0x00,
		0x00,
		0x70,
		0x70,
		0x70,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x93 */
		0x00,
		0x00,
		0x00,
		0x78,
		0x78,
		0x78,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x94*/
		0x00,
		0x00,
		0x00,
		0x7c,
		0x7c,
		0x7c,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x95 */
		0x00,
		0x00,
		0x00,
		0x7e,
		0x7e,
		0x7e,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x96 */
		0x00,
		0x00,
		0x00,
		0x7f,
		0x7f,
		0x7f,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x97*/
		0x00,
		0x00,
		0x00,
		0x1c,
		0x1c,
		0x1c,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x98 */
		0x1c,
		0x1c,
		0x1c,
		0x7f,
		0x7f,
		0x7f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x99 */
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f,
		0x7f
	},
	{	/* 0x9A */
		0x00,
		0x00,
		0x00,
		0x7c,
		0x7c,
		0x7c,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x9B */
		0x1c,
		0x1c,
		0x1c,
		0x1f,
		0x1f,
		0x1f,
		0x1c,
		0x1c,
		0x1c,
		0x1c
	},
	{	/* 0x9C */
		0x1c,
		0x1c,
		0x1c,
		0x7f,
		0x7f,
		0x7f,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x9D */
		0x1c,
		0x1c,
		0x1c,
		0x7c,
		0x7c,
		0x7c,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0x9E */
		0x40,
		0x60,
		0x30,
		0x10,
		0x08,
		0x08,
		0x04,
		0x06,
		0x06,
		0x02
	},
	{	/* 0x9F */
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22,
		0x22
	},
	{	/* 0xA0 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xA1 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x38,
		0x28,
		0x38,
		0x00,
		0x00
	},
	{	/* 0xA2 */
		0x00,
		0x0e,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xA3 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x08,
		0x08,
		0x08,
		0x38,
		0x00,
		0x00
	},
	{	/* 0xA4 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x10,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xA5 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x10,
		0x10,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xA6 */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x1e,
		0x02,
		0x04,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xA7 */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x02,
		0x0c,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xA8 */
		0x00,
		0x00,
		0x00,
		0x04,
		0x08,
		0x18,
		0x28,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xA9 */
		0x00,
		0x00,
		0x00,
		0x08,
		0x3e,
		0x22,
		0x02,
		0x0c,
		0x00,
		0x00
	},
	{	/* 0xAA */
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x08,
		0x08,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xAB */
		0x00,
		0x00,
		0x00,
		0x04,
		0x3e,
		0x0c,
		0x14,
		0x24,
		0x00,
		0x00
	},
	{	/* 0xAC */
		0x00,
		0x00,
		0x00,
		0x10,
		0x3e,
		0x12,
		0x14,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xAD */
		0x00,
		0x00,
		0x00,
		0x00,
		0x1c,
		0x04,
		0x04,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xAE */
		0x00,
		0x00,
		0x00,
		0x3c,
		0x04,
		0x3c,
		0x04,
		0x3c,
		0x00,
		0x00
	},
	{	/* 0xAF */
		0x00,
		0x00,
		0x00,
		0x00,
		0x2a,
		0x2a,
		0x02,
		0x0c,
		0x00,
		0x00
	},
	{	/* 0xB0 */
		0x00,
		0x00,
		0x00,
		0x00,
		0x3e,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xB1 */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x0c,
		0x08,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xB2 */
		0x00,
		0x04,
		0x08,
		0x18,
		0x28,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xB3 */
		0x00,
		0x08,
		0x3e,
		0x22,
		0x02,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xB4 */
		0x00,
		0x00,
		0x3e,
		0x08,
		0x08,
		0x08,
		0x08,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xB5 */
		0x00,
		0x04,
		0x3e,
		0x04,
		0x0c,
		0x14,
		0x24,
		0x04,
		0x00,
		0x00
	},
	{	/* 0xB6 */
		0x00,
		0x08,
		0x08,
		0x3e,
		0x0a,
		0x0a,
		0x12,
		0x24,
		0x00,
		0x00
	},
	{	/* 0xB7 */
		0x00,
		0x08,
		0x3e,
		0x08,
		0x3e,
		0x08,
		0x08,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xB8 */
		0x00,
		0x00,
		0x1e,
		0x12,
		0x22,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xB9 */
		0x00,
		0x10,
		0x10,
		0x1e,
		0x24,
		0x04,
		0x04,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xBA */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x02,
		0x02,
		0x02,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xBB */
		0x00,
		0x14,
		0x3e,
		0x14,
		0x14,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xBC */
		0x00,
		0x00,
		0x30,
		0x00,
		0x32,
		0x02,
		0x04,
		0x38,
		0x00,
		0x00
	},
	{	/* 0xBD */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x04,
		0x08,
		0x14,
		0x22,
		0x00,
		0x00
	},
	{	/* 0xBE */
		0x00,
		0x10,
		0x3e,
		0x12,
		0x14,
		0x10,
		0x10,
		0x1e,
		0x00,
		0x00
	},
	{	/* 0xBF */
		0x00,
		0x00,
		0x22,
		0x22,
		0x12,
		0x02,
		0x04,
		0x18,
		0x00,
		0x00
	},
	{	/* 0xC0 */
		0x00,
		0x00,
		0x1e,
		0x12,
		0x2a,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xC1 */
		0x00,
		0x04,
		0x38,
		0x08,
		0x3e,
		0x08,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xC2 */
		0x00,
		0x00,
		0x2a,
		0x2a,
		0x2a,
		0x02,
		0x04,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xC3 */
		0x00,
		0x00,
		0x1c,
		0x00,
		0x3e,
		0x08,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xC4 */
		0x00,
		0x10,
		0x10,
		0x10,
		0x18,
		0x14,
		0x10,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xC5 */
		0x00,
		0x08,
		0x08,
		0x3e,
		0x08,
		0x08,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xC6 */
		0x00,
		0x00,
		0x1c,
		0x00,
		0x00,
		0x00,
		0x00,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xC7 */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x14,
		0x08,
		0x14,
		0x20,
		0x00,
		0x00
	},
	{	/* 0xC8 */
		0x00,
		0x08,
		0x3e,
		0x04,
		0x08,
		0x1c,
		0x2a,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xC9 */
		0x00,
		0x04,
		0x04,
		0x04,
		0x04,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xCA */
		0x00,
		0x00,
		0x04,
		0x02,
		0x12,
		0x12,
		0x12,
		0x22,
		0x00,
		0x00
	},
	{	/* 0xCB */
		0x00,
		0x00,
		0x20,
		0x20,
		0x3e,
		0x20,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xCC */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x02,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xCD */
		0x00,
		0x00,
		0x10,
		0x28,
		0x04,
		0x02,
		0x02,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xCE */
		0x00,
		0x08,
		0x3e,
		0x08,
		0x2a,
		0x2a,
		0x2a,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xCF */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x02,
		0x14,
		0x08,
		0x04,
		0x00,
		0x00
	},
	{	/* 0xD0 */
		0x00,
		0x10,
		0x0e,
		0x00,
		0x0e,
		0x00,
		0x10,
		0x0e,
		0x00,
		0x00
	},
	{	/* 0xD1 */
		0x00,
		0x00,
		0x04,
		0x08,
		0x10,
		0x22,
		0x3e,
		0x02,
		0x00,
		0x00
	},
	{	/* 0xD2 */
		0x00,
		0x00,
		0x02,
		0x02,
		0x14,
		0x08,
		0x14,
		0x20,
		0x00,
		0x00
	},
	{	/* 0xD3 */
		0x00,
		0x00,
		0x3c,
		0x10,
		0x3e,
		0x10,
		0x10,
		0x1e,
		0x00,
		0x00
	},
	{	/* 0xD4 */
		0x00,
		0x10,
		0x10,
		0x3e,
		0x12,
		0x14,
		0x10,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xD5 */
		0x00,
		0x00,
		0x1c,
		0x04,
		0x04,
		0x04,
		0x04,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xD6 */
		0x00,
		0x00,
		0x3e,
		0x02,
		0x3e,
		0x02,
		0x02,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xD7 */
		0x00,
		0x00,
		0x1c,
		0x00,
		0x3e,
		0x02,
		0x02,
		0x0c,
		0x00,
		0x00
	},
	{	/* 0xD8 */
		0x00,
		0x00,
		0x12,
		0x12,
		0x12,
		0x02,
		0x04,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xD9 */
		0x00,
		0x00,
		0x28,
		0x28,
		0x28,
		0x2a,
		0x2a,
		0x2c,
		0x00,
		0x00
	},
	{	/* 0xDA */
		0x00,
		0x00,
		0x20,
		0x20,
		0x22,
		0x24,
		0x28,
		0x30,
		0x00,
		0x00
	},
	{	/* 0xDB */
		0x00,
		0x00,
		0x3e,
		0x22,
		0x22,
		0x22,
		0x22,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xDC */
		0x00,
		0x00,
		0x3e,
		0x22,
		0x22,
		0x02,
		0x04,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xDD */
		0x00,
		0x00,
		0x30,
		0x00,
		0x02,
		0x02,
		0x04,
		0x38,
		0x00,
		0x00
	},
	{	/* 0xDE */
		0x00,
		0x28,
		0x28,
		0x28,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xDF */
		0x00,
		0x38,
		0x28,
		0x38,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xE0 */
		0x00,
		0x1c,
		0x22,
		0x04,
		0x08,
		0x08,
		0x00,
		0x08,
		0x00,
		0x00
	},
	{	/* 0xE1 */
		0x00,
		0x18,
		0x18,
		0x18,
		0x18,
		0x00,
		0x18,
		0x18,
		0x00,
		0x00
	},
	{	/* 0xE2 */
		0x00,
		0x14,
		0x14,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xE3 */
		0x00,
		0x00,
		0x14,
		0x36,
		0x00,
		0x36,
		0x14,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xE4 */
		0x00,
		0x1c,
		0x2a,
		0x28,
		0x1c,
		0x0a,
		0x2a,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0xE5 */
		0x00,
		0x30,
		0x32,
		0x04,
		0x08,
		0x10,
		0x26,
		0x06,
		0x00,
		0x00
	},
	{	/* 0xE6 */
		0x00,
		0x10,
		0x28,
		0x28,
		0x10,
		0x2a,
		0x24,
		0x1a,
		0x00,
		0x00
	},
	{	/* 0xE7 */
		0x00,
		0x04,
		0x08,
		0x10,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xE8 */
		0x00,
		0x02,
		0x04,
		0x08,
		0x08,
		0x08,
		0x04,
		0x02,
		0x00,
		0x00
	},
	{	/* 0xE9 */
		0x00,
		0x20,
		0x10,
		0x08,
		0x08,
		0x08,
		0x10,
		0x20,
		0x00,
		0x00
	},
	{	/* 0xEA */
		0x00,
		0x00,
		0x08,
		0x1c,
		0x3e,
		0x1c,
		0x08,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xEB */
		0x00,
		0x00,
		0x08,
		0x08,
		0x3e,
		0x08,
		0x08,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xEC */
		0x00,
		0x00,
		0x00,
		0x00,
		0x18,
		0x18,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xED */
		0x00,
		0x00,
		0x00,
		0x00,
		0x3e,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xEE */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x18,
		0x18,
		0x00,
		0x00
	},
	{	/* 0xEF */
		0x00,
		0x00,
		0x02,
		0x04,
		0x08,
		0x10,
		0x20,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xF0 */
		0x00,
		0x0c,
		0x12,
		0x12,
		0x12,
		0x12,
		0x12,
		0x0c,
		0x00,
		0x00
	},
	{	/* 0xF1 */
		0x00,
		0x08,
		0x18,
		0x08,
		0x08,
		0x08,
		0x08,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0xF2 */
		0x00,
		0x1c,
		0x22,
		0x02,
		0x1c,
		0x20,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* 0xF3 */
		0x00,
		0x1c,
		0x22,
		0x02,
		0x0c,
		0x02,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0xF4 */
		0x00,
		0x04,
		0x0c,
		0x14,
		0x24,
		0x3e,
		0x04,
		0x04,
		0x00,
		0x00
	},
	{	/* 0xF5 */
		0x00,
		0x3e,
		0x20,
		0x3c,
		0x02,
		0x02,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0xF6 */
		0x00,
		0x0c,
		0x10,
		0x20,
		0x3c,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0xF7 */
		0x00,
		0x3e,
		0x02,
		0x04,
		0x08,
		0x10,
		0x10,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xF8 */
		0x00,
		0x1c,
		0x22,
		0x22,
		0x1c,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* 0xF9 */
		0x00,
		0x1c,
		0x22,
		0x22,
		0x1e,
		0x02,
		0x04,
		0x18,
		0x00,
		0x00
	},
	{	/* 0xFA */
		0x00,
		0x00,
		0x18,
		0x18,
		0x00,
		0x18,
		0x18,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xFB */
		0x00,
		0x18,
		0x18,
		0x00,
		0x18,
		0x18,
		0x08,
		0x10,
		0x00,
		0x00
	},
	{	/* 0xFC */
		0x00,
		0x02,
		0x04,
		0x08,
		0x10,
		0x08,
		0x04,
		0x02,
		0x00,
		0x00
	},
	{	/* 0xFD */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x00,
		0x3e,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* 0xFE */
		0x00,
		0x20,
		0x10,
		0x08,
		0x04,
		0x08,
		0x10,
		0x20,
		0x00,
		0x00
	},
	{	/* 0xFF */
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},


/* extra symbols for various national terminals, which use slightly modified
variants of the US character set */
	{	/* pound */
		0x00,
		0x0c,
		0x12,
		0x10,
		0x38,
		0x10,
		0x3a,
		0x34,
		0x00,
		0x00
	},
	{	/* yen */
		0x00,
		0x22,
		0x14,
		0x08,
		0x3e,
		0x08,
		0x3e,
		0x08,
		0x00,
		0x00
	},
	{	/* auml */
		0x00,
		0x00,
		0x22,
		0x00,
		0x1c,
		0x22,
		0x3e,
		0x22,
		0x00,
		0x00
	},
	{	/* Auml */
		0x00,
		0x22,
		0x00,
		0x1c,
		0x22,
		0x3e,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* Aring */
		0x00,
		0x08,
		0x00,
		0x1c,
		0x22,
		0x3e,
		0x22,
		0x22,
		0x00,
		0x00
	},
	{	/* uuml */
		0x00,
		0x00,
		0x22,
		0x00,
		0x22,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* aring */
		0x00,
		0x00,
		0x08,
		0x00,
		0x1c,
		0x22,
		0x3e,
		0x22,
		0x00,
		0x00
	},

	{	/* Uuml */
		0x00,
		0x22,
		0x00,
		0x22,
		0x22,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},

	{	/* ouml */
		0x00,
		0x00,
		0x22,
		0x00,
		0x3e,
		0x22,
		0x22,
		0x3e,
		0x00,
		0x00
	},
	{	/* Ouml */
		0x00,
		0x22,
		0x00,
		0x3e,
		0x22,
		0x22,
		0x22,
		0x3e,
		0x00,
		0x00
	},
	{	/* szlig */
		0x00,
		0x18,
		0x24,
		0x24,
		0x2c,
		0x22,
		0x22,
		0x2c,
		0x00,
		0x00
	},
	{	/* aelig */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x28,
		0x3e,
		0x28,
		0x2e,
		0x00,
		0x00
	},
	{	/* AElig */
		0x00,
		0x3e,
		0x28,
		0x28,
		0x3e,
		0x28,
		0x28,
		0x2e,
		0x00,
		0x00
	},
	{	/* Oslash */
		0x00,
		0x00,
		0x00,
		0x3e,
		0x26,
		0x2a,
		0x32,
		0x3e,
		0x00,
		0x00
	},
	{	/* oslash */
		0x00,
		0x3e,
		0x22,
		0x26,
		0x2a,
		0x32,
		0x22,
		0x3e,
		0x00,
		0x00
	},
/* WARNING: I have created the next 8 French character matrices from scratch,
as I have no printout of the original matrices. */
	{	/* agrave */
		0x00,
		0x10,
		0x08,
		0x00,
		0x1c,
		0x22,
		0x3e,
		0x22,
		0x00,
		0x00
	},
	{	/* deg */
		0x00,
		0x38,
		0x28,
		0x38,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	},
	{	/* ccedil */
		0x00,
		0x00,
		0x00,
		0x1e,
		0x20,
		0x20,
		0x20,
		0x1e,
		0x08,
		0x04
	},
	{	/* sect */
		0x00,
		0x1c,
		0x20,
		0x1c,
		0x22,
		0x1c,
		0x02,
		0x1c,
		0x00,
		0x00
	},
	{	/* egrave */
		0x00,
		0x10,
		0x08,
		0x3e,
		0x20,
		0x3c,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* ugrave */
		0x00,
		0x10,
		0x08,
		0x22,
		0x22,
		0x22,
		0x22,
		0x1c,
		0x00,
		0x00
	},
	{	/* eacute */
		0x00,
		0x04,
		0x08,
		0x3e,
		0x20,
		0x3c,
		0x20,
		0x3e,
		0x00,
		0x00
	},
	{	/* uml */
		0x00,
		0x22,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	}
};

