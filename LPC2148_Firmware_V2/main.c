#include <lpc214x.h>
#include <stdio.h>
#include <math.h>

// --- Pin Definitions ---
// Port 0
#define RELAY_PIN (1 << 11)   // P0.11 (Active Safety Relay)
#define LCD_RS    (1 << 16)   // P0.16
#define LCD_RW    (1 << 17)   // P0.17
#define LCD_EN    (1 << 18)   // P0.18
#define LCD_D4    (1 << 19)   // P0.19
#define LCD_D5    (1 << 20)   // P0.20
#define LCD_D6    (1 << 21)   // P0.21
#define LCD_D7    (1 << 22)   // P0.22

// Port 1
#define BAL1_PIN  (1 << 16)   // P1.16 (Opto Cell 1)
#define BAL2_PIN  (1 << 17)   // P1.17 (Opto Cell 2)
#define BAL3_PIN  (1 << 18)   // P1.18 (Opto Cell 3)

#define INA226_ADDR 0x80  // 0x40 standard -> 0x80 shifted for ARM 8-bit I2C

// --- Delay Function ---
void delay_us(unsigned int us) {
    unsigned int i;
    for(i=0; i<(us*12); i++); 
}
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++)
        for(j=0; j<12000; j++); 
}

// --- 2. UART1 Telemetry Setup (Pin P0.8) ---
void UART1_Init(void) {
    PINSEL0 |= 0x00050000; // Enable TxD1 (P0.8) and RxD1 (P0.9)
    U1LCR = 0x83; // 8 bits, no Parity, 1 Stop bit, DLAB = 1
    U1DLL = 98;   // 9600 Baud Rate @ 15MHz VPB Clock
    U1DLM = 0;
    U1LCR = 0x03; // DLAB = 0
}

void UART1_SendChar(char ch) {
    while (!(U1LSR & 0x20)); 
    U1THR = ch;
}

void UART1_SendString(char* str) {
    while (*str) UART1_SendChar(*str++);
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

// --- I2C0 Functions (INA226) ---
void I2C0_Init(void) {
    PINSEL0 |= 0x00000050; // SCL0 -> P0.2, SDA0 -> P0.3
    I2C0SCLH = 75; I2C0SCLL = 75;
    I2C0CONSET = 0x40; // Master Enable
}

#define I2C_WAIT() { int t = 200000; while(((I2C0CONSET & 0x08) == 0) && --t); if(t==0) { I2C0CONCLR=0x28; return 0; } }

unsigned int INA226_ReadReg(unsigned char reg) {
    unsigned int data = 0;
    
    I2C0CONSET = 0x20; I2C_WAIT(); // Send START
    I2C0CONCLR = 0x20;             // Clear START bit
    
    I2C0DAT = INA226_ADDR; I2C0CONCLR = 0x08; I2C_WAIT(); // Send Address
    if(I2C0STAT != 0x18) { I2C0CONSET = 0x10; I2C0CONCLR = 0x08; return 0; } // Abort on NACK
    
    I2C0DAT = reg; I2C0CONCLR = 0x08; I2C_WAIT(); // Send Register
    if(I2C0STAT != 0x28) { I2C0CONSET = 0x10; I2C0CONCLR = 0x08; return 0; } // Abort on NACK
    
    I2C0CONSET = 0x20; I2C0CONCLR = 0x08; I2C_WAIT(); // Send REP_START
    I2C0CONCLR = 0x20;             // Clear START bit
    
    I2C0DAT = INA226_ADDR | 0x01; I2C0CONCLR = 0x08; I2C_WAIT(); // Send Address+Read
    if(I2C0STAT != 0x40) { I2C0CONSET = 0x10; I2C0CONCLR = 0x08; return 0; } // Abort on NACK
    
    I2C0CONSET = 0x04; I2C0CONCLR = 0x08; I2C_WAIT(); // Receive Byte 1, send ACK
    data = (I2C0DAT << 8); 
    
    I2C0CONCLR = 0x04; I2C0CONCLR = 0x08; I2C_WAIT(); // Receive Byte 2, send NACK
    data |= I2C0DAT; 
    
    I2C0CONSET = 0x10; I2C0CONCLR = 0x08; // Send STOP
    return data;
}

// --- Modular ADC Expansion (Channels 0-4) ---
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
    
    // Start Conversion
    AD0CR |= 0x01000000; 
    
    // Poll the Global Data Register (AD0GDR) for the DONE bit
    do { 
        adc_data = AD0GDR; 
    } while((adc_data & 0x80000000) == 0);
    
    // Stop conversion
    AD0CR &= ~0x01000000; 
    return (adc_data >> 6) & 0x3FF; 
}

// Hardware Physical Oversampling Helper (10 instantaneous shots to kill EMI noise)
float Get_Stable_ADC(unsigned char channel) {
    int sum = 0;
    int i;
    for(i=0; i<10; i++) {
        sum += Read_ADC(channel);
    }
    return sum / 10.0;
}

// --- Main Program ---
int main(void) {
    char lcdBuffer[20];
    char jsonBuffer[200];
    
    unsigned int raw_pack_v;
    short raw_shunt_v; 
    
    float pack_voltage, temp_current, live_power;
    float ntc_voltage, temp_c;
    float tap1, tap2, tap3;
    float cell1, cell2, cell3;
    
    int bal1 = 0, bal2 = 0, bal3 = 0, relay_state = 1;
    char isDischarging = 0;

    // PRECISE Calibrated Multipliers (Mapped from real multimeter readings: 4.08V per cell)
    float CAL_1 = 0.9933; 
    float CAL_2 = 0.9864; 
    float CAL_3 = 0.9981; 

    // GPIO Configuration
    IODIR0 |= RELAY_PIN;
    IODIR1 |= (BAL1_PIN | BAL2_PIN | BAL3_PIN);
    
    IOSET0 = RELAY_PIN; // Start SAFE
    IOCLR1 = BAL1_PIN | BAL2_PIN | BAL3_PIN; 

    lcd_init();
    I2C0_Init();
    ADC_Init();
    UART1_Init();
    
    lcd_cmd(0x80); 
    lcd_print("BMS V2 Online   ");
    delay_ms(1500); 

    while(1) {
        // --- 1. Get High-Precision Current & Pack V via INA226 ---
        raw_pack_v = INA226_ReadReg(0x02);
        pack_voltage = raw_pack_v * 0.00125;
        
        raw_shunt_v = (short)INA226_ReadReg(0x01); 
        temp_current = (raw_shunt_v * 0.0000025) / 0.01;
        live_power = pack_voltage * temp_current;
        
        // Active load check to prevent balancing while discharging
        isDischarging = (temp_current > 0.15) ? 1 : 0; 

        // --- 2. Isolation & ADC Polling ---
        // Turn OFF balancers purely to silence EMI noise before reading ADC
        IOCLR1 = BAL1_PIN | BAL2_PIN | BAL3_PIN;
        delay_us(500); // Settle time

        // Read ADCs using 10-shot oversampling (10-bit math: 3.3/1023)
        // Ch4=NTC (P0.25), Ch1=Tap1 (P0.28), Ch2=Tap2 (P0.29), Ch3=Tap3 (P0.30)
        ntc_voltage = Get_Stable_ADC(4) * (3.3 / 1023.0);
        
        tap1 = (Get_Stable_ADC(1) * (3.3 / 1023.0)) * 1.4769 * CAL_1; 
        tap2 = (Get_Stable_ADC(2) * (3.3 / 1023.0)) * 3.2359 * CAL_2;
        tap3 = (Get_Stable_ADC(3) * (3.3 / 1023.0)) * 4.0461 * CAL_3;
        
        // --- 3. Compute Real Values ---
        temp_c = 0.0;
        if (ntc_voltage < 3.2 && ntc_voltage > 0.1) {
            float r_ntc = 10000.0 * ntc_voltage / (3.3 - ntc_voltage);
            temp_c = 1.0 / (1.0 / (25.0 + 273.15) + (1.0 / 3950.0) * log(r_ntc / 10000.0)) - 273.15;
        }

        cell1 = tap1;
        cell2 = tap2 - tap1;
        cell3 = tap3 - tap2;
        
        if(cell2 < 0) cell2 = 0; if(cell3 < 0) cell3 = 0;

        // --- 4. Logic Control (Dual-Threshold UVLO) ---
        // Over-Current or Absolute Over-Voltage check
        if(temp_current > 8.0 || pack_voltage > 12.7) {
            relay_state = 0; // FAULT
        } 
        // Under-Voltage Lockout (UVLO) Check
        else if (isDischarging) {
            // Active Load State: Allow temporary voltage sag down to 8.5V
            if (pack_voltage < 8.5) relay_state = 0; 
            else relay_state = 1;
        } else {
            // Idle State: Battery is resting, cut off at true empty (9.6V)
            if (pack_voltage < 9.6) relay_state = 0;
            else relay_state = 1;
        }

        // Apply physical relay hardware state
        if (relay_state == 1) IOSET0 = RELAY_PIN;
        else IOCLR0 = RELAY_PIN;

        bal1 = (cell1 > 4.15 && !isDischarging) ? 1 : 0;
        bal2 = (cell2 > 4.15 && !isDischarging) ? 1 : 0;
        bal3 = (cell3 > 4.15 && !isDischarging) ? 1 : 0;
        
        // Restore dynamic balancer state
        if(bal1) IOSET1 = BAL1_PIN; else IOCLR1 = BAL1_PIN;
        if(bal2) IOSET1 = BAL2_PIN; else IOCLR1 = BAL2_PIN;
        if(bal3) IOSET1 = BAL3_PIN; else IOCLR1 = BAL3_PIN;

        // --- 5. UI LCD Override ---
        lcd_cmd(0x80); 
        sprintf(lcdBuffer, "C1:%.2f C2:%.2f ", cell1, cell2);
        lcd_print(lcdBuffer);
        
        lcd_cmd(0xC0); 
        sprintf(lcdBuffer, "C3:%.2f I:%.1fA ", cell3, temp_current);
        lcd_print(lcdBuffer);

        // --- 6. Telemetry Blast (JSON for ESP32 Dashboard) ---
        sprintf(jsonBuffer, "{\"V\":%.2f,\"I\":%.2f,\"P\":%.2f,\"C1\":%.2f,\"C2\":%.2f,\"C3\":%.2f,\"NTC\":%.2f,\"R\":%d,\"B\":[%d,%d,%d]}\n", 
                pack_voltage, temp_current, live_power, cell1, cell2, cell3, temp_c, relay_state, bal1, bal2, bal3);
        
        UART1_SendString(jsonBuffer);

        delay_ms(1000); 
    }
}
