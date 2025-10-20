#include "stm32g0xx.h"
#include "stdio.h"
#include <string.h>
#include "jsmn.h"
#include "uart.h"
#include "jsonprocess.h"

int main() 
{
	uart_init();
	
	while(1)
	{
		json_process();
	}
	
	return 0;
}
 
