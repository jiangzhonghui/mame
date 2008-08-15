#define HD63484_RAM_SIZE 0x80000

extern UINT16 *HD63484_ram;
extern UINT16 HD63484_reg[256/2];

void HD63484_start(void);

READ16_HANDLER( HD63484_status_r );
WRITE16_HANDLER( HD63484_address_w );
WRITE16_HANDLER( HD63484_data_w );
READ16_HANDLER( HD63484_data_r );
