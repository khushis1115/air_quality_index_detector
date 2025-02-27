#include <lpc17xx.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#define RS_CTRL 0x00000100 // P0.8
#define EN_CTRL 0x10000200 // P0.9
#define DT_CTRL 0x000000F0 // P0.4 to P0.7

unsigned long int init_command[] = {0x30, 0x30, 0x30, 0x20, 0x28, 0x0C, 0x06, 0x01, 0x80}; // Initialization commands
unsigned long int temp1 = 0, temp2 = 0, i, j, var1, var2;
unsigned char flag1 = 0, flag2 = 0;
unsigned char msg[] = {"QUALITY:"}; 
unsigned char msg2[] = {"AQI: "};

void lcd_init(void); 
void lcd_write(void); 
void port_write(void); 
void delay(unsigned int); 
void lcd_print_msg(void); 
void lcd_print_msg2(void); 
void lcd_print_aval(void); 
void lcd_print_dval(void);

int main(void) {
    unsigned int mqReading;
    char disp_qual[6]; // Quality level
    char digitalValStr[14]; // AQI value

    SystemInit();
    SystemCoreClockUpdate();

    LPC_PINCON->PINSEL1 |= 1 << 14; // Function 1 on P0.23 for AD0.0
    LPC_SC->PCONP |= (1 << 12); // Power supply for ADC

    LPC_GPIO0->FIODIR = DT_CTRL | RS_CTRL | EN_CTRL;

    lcd_init();
    lcd_print_msg();
    lcd_print_msg2();

    LPC_GPIO2->FIODIR = 1 << 13; // Buzzer on P2.13

    while (1) {
        LPC_ADC->ADCR = (1 << 0) | (1 << 21) | (1 << 24); // Channel 0, power on, software mode start

        while (((mqReading = LPC_ADC->ADGDR) & 0x80000000) == 0); // Waiting for done bit to get high

        mqReading = (LPC_ADC->ADGDR >> 4) & 0x00000FFF;
        sprintf(digitalValStr, "%d", (mqReading / 7)); // Digital voltage

        if ((mqReading / 7) >= 490) {
            strcpy(disp_qual, "DANGER");
        } else if ((mqReading / 7) >= 300 && (mqReading / 7) <= 489) {
            strcpy(disp_qual, "BAD");
        } else {
            strcpy(disp_qual, "GOOD");
        }

        temp1 = 0x89;
        flag1 = 0;
        lcd_write();
        delay(800);

        i = 0;
        flag1 = 1;
        while (disp_qual[i] != '\0') {
            temp1 = disp_qual[i];
            lcd_write();
            i += 1;
        }

        temp1 = 0xC5; // Change line
        flag1 = 0;
        lcd_write();
        delay(800);

        i = 0;
        flag1 = 1;
        while (digitalValStr[i] != '\0') {
            temp1 = digitalValStr[i];
            lcd_write();
            i += 1;
        }

        if ((mqReading / 7) >= 450) { // Activate buzzer for 'DANGER'
            LPC_GPIO2->FIOPIN = 1 << 13;
            delay(500000);
            LPC_GPIO2->FIOCLR = 1 << 13;
        }
    }
}

void lcd_init(void) {
    unsigned int x;
    flag1 = 0; // Command Mode

    for (x = 0; x < 9; x++) {
        temp1 = init_command[x];
        lcd_write();
    }

    flag1 = 1; // Data Mode
}

void lcd_write(void) {
    flag2 = (flag1 == 1) ? 0 : ((temp1 == 0x30) || (temp1 == 0x20)) ? 1 : 0;

    temp2 = temp1 & 0xF0;
    port_write();

    if (flag2 == 0) { // Send least significant 4 bits only when it's data/command other than 0x30/0x20
        temp2 = (temp1 & 0x0F) << 4;
        port_write();
    }
}

void port_write(void) {
    LPC_GPIO0->FIOPIN = temp2;

    if (flag1 == 0) {
        LPC_GPIO0->FIOCLR = RS_CTRL;
    } else {
        LPC_GPIO0->FIOSET = RS_CTRL;
    }

    LPC_GPIO0->FIOSET = EN_CTRL;
    delay(25);
    LPC_GPIO0->FIOCLR = EN_CTRL;
    delay(30000);
}

void delay(unsigned int rl) {
    unsigned int r;
    for (r = 0; r < rl; r++);
}

void lcd_print_msg(void) {
    unsigned int a;
    for (a = 0; msg[a] != '\0'; a++) {
        temp1 = msg[a];
        lcd_write();
    }
}

void lcd_print_msg2(void) { // Change line to print second message
    temp1 = 0xC0;
    flag1 = 0;
    lcd_write();
    delay(800);

    i = 0;
    flag1 = 1;
    while (msg2[i] != '\0') {
        temp1 = msg2[i];
        lcd_write();
        i += 1;
    }
}
