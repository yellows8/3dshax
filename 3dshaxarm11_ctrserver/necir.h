#define NEC_IR_ADDRESS_LG_TV 0x04

// The actual remote key command codes we implement here are as
// follows. (Of course, I didn't really _need_ any other than 
// the IN_START and the EZ_ADJUST codes, but the others were a 
// nice thing to have for initial testing.)

#define IRKEY_CHANNEL_PLUS  0x00
#define IRKEY_CHANNEL_MINUS 0x01
#define IRKEY_VOLUME_PLUS   0x02
#define IRKEY_VOLUME_MINUS  0x03
#define IRKEY_MUTE          0x09
#define IRKEY_Q_MENU        0x45
#define IRKEY_IN_START      0xFB
#define IRKEY_EZ_ADJUST     0xFF

void transmit_nec_ir_command(uint8_t address, uint8_t command);

