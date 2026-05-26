#include <lpc214x.h>
#include <stdio.h>

// --- Pin Definitions ---
#define LCD_RS    (1 << 16)   // P0.16
#define LCD_RW    (1 << 17)   // P0.17
#define LCD_EN    (1 << 18)   // P0.18
#define LCD_D4    (1 << 19)   // P0.19
#define LCD_D5    (1 << 20)   // P0.20
#define LCD_D6    (1 << 21)   // P0.21
#define LCD_D7    (1 << 22)   // P0.22

#define BAL1_PIN  (1 << 16)   // P1.16 
#define BAL2_PIN  (1 << 17)   // P1.17 
#define BAL3_PIN  (1 << 18)   // P1.18 

// --- Delay Function ---
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++) for(j=0; j<12000; j++); 
}

// --- LCD Core Functions ---
void lcd_cmd(unsigned char cmd) {
    IOCLR0 = LCD_RS; IOCLR0 = LCD_RW; 
    if(cmd & 0x80) IOSET0 = LCD_D7; else IOCLR0 = LCD_D7;
    if(cmd & 0x40) IOSET0 = LCD_D6; else IOCLR0 = LCD_D6;
    if(cmd & 0x20) IOSET0 = LCD_D5; else IOCLR0 = LCD_D5;
    if(cmd & 0x10) IOSET0 = LCD_D4; else IOCLR0 = LCD_D4;
    IOSET0 = LCD_EN; delay_ms(2); IOCLR0 = LCD_EN;
    if(cmd & 0x08) IOSET0 = LCD_D7; else IOCLR0 = LCD_D7;
    if(cmd & 0x04) IOSET0 = LCD_D6; else IOCLR0 = LCD_D6;
    if(cmd & 0x02) IOSET0 = LCD_D5; else IOCLR0 = LCD_D5;
    if(cmd & 0x01) IOSET0 = LCD_D4; else IOCLR0 = LCD_D4;
    IOSET0 = LCD_EN; delay_ms(2); IOCLR0 = LCD_EN;
}
void lcd_data(unsigned char data) {
    IOSET0 = LCD_RS; IOCLR0 = LCD_RW; 
    if(data & 0x80) IOSET0 = LCD_D7; else IOCLR0 = LCD_D7;
    if(data & 0x40) IOSET0 = LCD_D6; else IOCLR0 = LCD_D6;
    if(data & 0x20) IOSET0 = LCD_D5; else IOCLR0 = LCD_D5;
    if(data & 0x10) IOSET0 = LCD_D4; else IOCLR0 = LCD_D4;
    IOSET0 = LCD_EN; delay_ms(2); IOCLR0 = LCD_EN;
    if(data & 0x08) IOSET0 = LCD_D7; else IOCLR0 = LCD_D7;
    if(data & 0x04) IOSET0 = LCD_D6; else IOCLR0 = LCD_D6;
    if(data & 0x02) IOSET0 = LCD_D5; else IOCLR0 = LCD_D5;
    if(data & 0x01) IOSET0 = LCD_D4; else IOCLR0 = LCD_D4;
    IOSET0 = LCD_EN; delay_ms(2); IOCLR0 = LCD_EN;
}
void lcd_init(void) {
    PINSEL1 &= 0xFFFFC000; 
    IODIR0 |= (LCD_RS | LCD_RW | LCD_EN | LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7);
    delay_ms(20);
    lcd_cmd(0x02); lcd_cmd(0x28); lcd_cmd(0x0C); lcd_cmd(0x01); delay_ms(5);
}
void lcd_print(char *str) { while(*str) lcd_data(*str++); }

// --- ADC Functions ---
void ADC_Init(void) {
    // Map P0.25 (AD0.4), P0.28 (AD0.1), P0.29 (AD0.2), P0.30 (AD0.3)
    PINSEL1 |= 0x15040000; 
    PINSEL1 &= ~0x2A080000;
}

unsigned int Read_ADC(unsigned char channel) {
    unsigned int adc_data;
    
    if (channel > 4) return 0;
    
    // Configure AD0CR: Select channel, CLKDIV=15, PDN=1
    AD0CR = 0x00200F00 | (1 << channel);    
    
    // Start Conversion (Bit 24)
    AD0CR |= 0x01000000; 
    
    // Poll the Global Data Register (AD0GDR) for the DONE bit (Bit 31)
    do { 
        adc_data = AD0GDR; 
    } while((adc_data & 0x80000000) == 0);
    
    // Stop conversion
    AD0CR &= ~0x01000000; 
    
    // Extract 10-bit result
    return (adc_data >> 6) & 0x3FF; 
}

float Get_Stable_ADC(unsigned char channel) {
    int sum = 0;
    int i;
    for(i=0; i<10; i++) sum += Read_ADC(channel);
    return sum / 10.0;
}

// --- Main Program ---
int main(void) {
    char lcdBuffer1[16];
    char lcdBuffer2[16];
    
    float tap1, tap2, tap3, ntc_voltage;

    // PRECISE Calibrated Multipliers
    float CAL_1 = 1.0371; 
    float CAL_2 = 1.0541; 
    float CAL_3 = 0.9957; 

    // Turn off balancers so they don't drag voltage down
    IODIR1 |= (BAL1_PIN | BAL2_PIN | BAL3_PIN);
    IOCLR1 = BAL1_PIN | BAL2_PIN | BAL3_PIN; 

    lcd_init();
    ADC_Init();
    
    lcd_cmd(0x80); 
    lcd_print("ADC DEBUG MODE  ");
    delay_ms(1500); 

    while(1) {
        // Read ADCs (10-shot oversampling + 10-bit math: 3.3/1023)
        ntc_voltage = Get_Stable_ADC(4) * (3.3 / 1023.0);
        
        tap1 = (Get_Stable_ADC(1) * (3.3 / 1023.0)) * 1.4769 * CAL_1; 
        tap2 = (Get_Stable_ADC(2) * (3.3 / 1023.0)) * 3.2359 * CAL_2;
        tap3 = (Get_Stable_ADC(3) * (3.3 / 1023.0)) * 4.0461 * CAL_3;

        // Display Raw Tap Voltages on LCD
        lcd_cmd(0x80); // Row 1
        sprintf(lcdBuffer1, "T1:%.2f T2:%.2f", tap1, tap2);
        lcd_print(lcdBuffer1);
        
        lcd_cmd(0xC0); // Row 2
        sprintf(lcdBuffer2, "T3:%.2f NT:%.2f", tap3, ntc_voltage);
        lcd_print(lcdBuffer2);

        delay_ms(600); 
    }
}
