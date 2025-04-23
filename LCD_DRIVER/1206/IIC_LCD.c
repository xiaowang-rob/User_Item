#include "I2C_LCD.h"

// change your handler here accordingly
#define SLAVE_ADDRESS_LCD 0x4e // change this according to ur setup

void lcd_send_cmd(char cmd, I2C_HandleTypeDef hi2c)
{ // 0011 0000

	char data_u, data_l;
	uint8_t i2c_frame_data[4];
	data_u = (cmd & 0xf0);			   // 0011 0000
	data_l = ((cmd << 4) & 0xf0);	   // 0000 0000
	i2c_frame_data[0] = data_u | 0x0C; // en=1, rs=0	//0011 1100
	i2c_frame_data[1] = data_u | 0x08; // en=0, rs=0	//0011 1000
	i2c_frame_data[2] = data_l | 0x0C; // en=1, rs=0	//0000 1100
	i2c_frame_data[3] = data_l | 0x08; // en=0, rs=0	//0000 1000
	if (HAL_I2C_Master_Transmit(&hi2c, SLAVE_ADDRESS_LCD, (uint8_t *)i2c_frame_data, 4, 100) != HAL_OK)
		Error_Handler();
	HAL_Delay(10);
}

void lcd_send_data(char data, I2C_HandleTypeDef hi2c)
{ // 1111 0000
	char data_u, data_l;
	uint8_t i2c_frame_data[4];
	data_u = (data & 0xf0);			   // 1111 0000
	data_l = ((data << 4) & 0xf0);	   // 0000 0000
	i2c_frame_data[0] = data_u | 0x0D; // en=1, rs=0//1111 1101
	i2c_frame_data[1] = data_u | 0x09; // en=0, rs=0//1111 1001
	i2c_frame_data[2] = data_l | 0x0D; // en=1, rs=0//0000 1101
	i2c_frame_data[3] = data_l | 0x09; // en=0, rs=0//0000 1001
	if (HAL_I2C_Master_Transmit(&hi2c, SLAVE_ADDRESS_LCD, (uint8_t *)i2c_frame_data, 4, 100) != HAL_OK)
		Error_Handler();

	HAL_Delay(10);
}

void lcd_clear(I2C_HandleTypeDef hi2c)
{
	lcd_send_cmd(0x01, hi2c); // clear display
	HAL_Delay(100);
}

void lcd_put_cur(int row, int col, I2C_HandleTypeDef hi2c)
{
	switch (row)
	{
	case 0:
		col |= 0x80;
		break;
	case 1:
		col |= 0xc0;
		break;
	}
	lcd_send_cmd(col, hi2c);
}

void lcd_init(I2C_HandleTypeDef hi2c)
{
	// 4-bit mode initialization

	HAL_Delay(500); // wait for >40ms
	lcd_send_cmd(0x02, hi2c);
	HAL_Delay(1);
	lcd_send_cmd(0x28, hi2c);
	HAL_Delay(5); // wait for > 4.1ms
				  //	lcd_send_cmd(0x30,hi2c);
				  //	HAL_Delay(10);
				  //	lcd_send_cmd(0x20,hi2c); // 4-bit mode
				  //	HAL_Delay(10);

	//    // display initialization
	//	lcd_send_cmd(0x28,hi2c); // Function set --> DL = 0 (4-bit mode), N = 1 (2 line display), F = 0 (5x8 characters)
	//	HAL_Delay(1);
	//	lcd_send_cmd(0x28,hi2c); // Function set --> DL = 0 (4-bit mode), N = 1 (2 line display), F = 0 (5x8 characters)
	//	HAL_Delay(1);
	//	lcd_send_cmd(0x08,hi2c); // Display on/off control --> D = 0, C = 0, B = 0 --> display off
	//	HAL_Delay(50);
	lcd_send_cmd(0x01, hi2c); // clear display
	HAL_Delay(10);
	lcd_send_cmd(0x06, hi2c); // Enter mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	HAL_Delay(1);
	lcd_send_cmd(0x0c, hi2c); // Display on /off control --> D = 1, C = 1, B = 0

	HAL_Delay(100);
}

void lcd_send_string(char *str, I2C_HandleTypeDef hi2c)
{
	while (*str)
	{
		lcd_send_data(*str++, hi2c);
	}
	HAL_Delay(1);
}
