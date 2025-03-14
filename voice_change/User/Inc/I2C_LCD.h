#include "main.h"

void lcd_init(I2C_HandleTypeDef hi2c);
void lcd_send_cmd(char cmd,I2C_HandleTypeDef hi2c);
void lcd_send_data(char data,I2C_HandleTypeDef hi2c);
void lcd_send_string(char *str,I2C_HandleTypeDef hi2c);
void lcd_put_cur(int row,int col,I2C_HandleTypeDef hi2c);
void lcd_clear(I2C_HandleTypeDef hi2c);
