#include "led_display.h"
#include "software_timer.h"

/* Wiring (matches the Proteus schematic and Lab_3.ioc):
 *  7-seg segments a..g : SEG0..SEG6 = PB0..PB6  (common anode, ACTIVE LOW)
 *  digit enables 0..3  : EN0..EN3   = PA9..PA12 (PNP high-side, ACTIVE LOW)
 *  traffic LEDs (ACTIVE HIGH):
 *      road A (left/right): L2 = red (PA5), L1 = amber (PA4), L0 = green (PA3)
 *      road B (top/bottom): L5 = red (PA8), L4 = amber (PA7), L3 = green (PA6)
 *  digit 0,1 = left value (tens, units), digit 2,3 = right value
 */
#define DISPLAY_SCAN_MS   5      /* with TICK_MS = 10 this rounds up to 10 ms */
#define NO_OF_DIGITS      4

/* Pins come from main.h (generated from Lab_3.ioc) */
#define SEG_MASK   (SEG0_Pin | SEG1_Pin | SEG2_Pin | SEG3_Pin | \
                    SEG4_Pin | SEG5_Pin | SEG6_Pin)           /* GPIOB */
#define EN_MASK    (EN0_Pin | EN1_Pin | EN2_Pin | EN3_Pin)    /* GPIOA */
#define LIGHT_MASK (L0_Pin | L1_Pin | L2_Pin | L3_Pin | L4_Pin | L5_Pin) /* GPIOA */

static const uint16_t enPin[NO_OF_DIGITS] = { EN0_Pin, EN1_Pin, EN2_Pin, EN3_Pin };

/* lightPin[road][color], color: 0 = red, 1 = amber, 2 = green */
static const uint16_t lightPin[2][3] = {
    { L2_Pin, L1_Pin, L0_Pin },   /* road A */
    { L5_Pin, L4_Pin, L3_Pin }    /* road B */
};

/* bit0 = a ... bit6 = g */
static const uint8_t seg7[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static uint8_t displayBuffer[NO_OF_DIGITS];
static uint8_t scanIndex = 0;

void display_init(void) {
    for (uint8_t i = 0; i < NO_OF_DIGITS; i++) displayBuffer[i] = 0;
    GPIOA->BSRR = EN_MASK;      /* all digits OFF (PNP: pin HIGH) */
    GPIOB->BSRR = SEG_MASK;     /* all segments OFF (common anode: pin HIGH) */
    lights_off();
    setTimer(TIMER_SCAN, DISPLAY_SCAN_MS);
}

void display_set(uint8_t left, uint8_t right) {
    if (left > 99)  left = 99;
    if (right > 99) right = 99;
    displayBuffer[0] = left / 10;
    displayBuffer[1] = left % 10;
    displayBuffer[2] = right / 10;
    displayBuffer[3] = right % 10;
}

static void display_digit(uint8_t index) {
    /* PNP transistor + common-anode display: everything is ACTIVE LOW */
    GPIOA->BSRR = EN_MASK;                                     /* all digits off (EN = 1) */
    uint32_t p = seg7[displayBuffer[index]] & 0x7FUL;          /* bit i = SEGi, PB0..PB6  */
    GPIOB->BSRR = (p << 16) | ((~p) & 0x7FUL);                 /* lit segment -> pin LOW  */
    GPIOA->BSRR = ((uint32_t)enPin[index]) << 16;              /* enable one digit (EN = 0) */
}

void display_run(void) {
    if (isTimerExpired(TIMER_SCAN)) {
        setTimer(TIMER_SCAN, DISPLAY_SCAN_MS);
        display_digit(scanIndex);
        scanIndex = (scanIndex + 1) % NO_OF_DIGITS;
    }
}

void road_light(uint8_t road, Light_t color) {
    if (road > 1) return;
    uint32_t all = lightPin[road][0] | lightPin[road][1] | lightPin[road][2];
    /* reset the 3 LEDs of this road and set one; BSRR set-bit wins over reset-bit */
    GPIOA->BSRR = (all << 16) | lightPin[road][color];
}

void lights_off(void) {
    GPIOA->BSRR = ((uint32_t)LIGHT_MASK) << 16;
}

void blink_color(Light_t color, uint8_t on) {
    uint32_t setBits = 0;
    if (on) setBits = lightPin[0][color] | lightPin[1][color];
    GPIOA->BSRR = (((uint32_t)LIGHT_MASK) << 16) | setBits;
}
