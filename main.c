#include "stdio.h"
#include "jsonprocess.h"

int main() 
{
	json_process_init();
	
	while(1)
	{
		json_process();
	}
	
	return 0;
}
 
