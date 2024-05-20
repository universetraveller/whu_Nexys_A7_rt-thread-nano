#include "applications.h"
#include "port_utils.h"
#include "time.h"
#include "common_types.h"
// Max input is 99999999
#define BITS_PER_POS 4
#define MASK2 10
#define MASK3 100
#define MASK4 1000
#define MASK5 10000
#define MASK6 100000
#define MASK7 1000000
#define MASK8 10000000
#define MASK_INT(mask) if(from >= mask) {to += from / mask; from %= mask;} to <<= BITS_PER_POS;
u32_t convert_decimal_to_hex(u32_t from)
{
    u32_t to = 0;
    MASK_INT(MASK8)
    MASK_INT(MASK7)
    MASK_INT(MASK6)
    MASK_INT(MASK5)
    MASK_INT(MASK4)
    MASK_INT(MASK3)
    MASK_INT(MASK2)
    return to + from;
}

/*
    Implement decimal to hex
    Make 7-segment led display show number in decimal
*/
void display_in_decimal(u32_t num)
{
    digit_display_num(convert_decimal_to_hex(num));
}

static u16_t ALARM_LEDS = 0xFFFF;
void alarm_using_leds(void)
{
    led_display_16(ALARM_LEDS);
    ALARM_LEDS = ~ALARM_LEDS;
}

static int position_map[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
void init_position_map(void)
{
    int pos = 0;
    for(; pos < 16; ++ pos)
        position_map[pos] = 0b1 << pos;

}

#define CHECK_POS(ctl, pos) if(*ctl & position_map[pos]) {*ctl = pos; return;}
void parse_control(u32_t* ctl)
{
    int pos = 15;
    for(; pos > -1; -- pos)
        CHECK_POS(ctl, pos)
}