#include "ap_version.h"
#include "project.h"

#define DBG_ON 	1

INT8U* cmpute0r_build_time_get(void)
{
	INT8U temp[] = __DATE__ ;
	INT8U *temp0;
	INT8U month[4]={0};
	INT8U date[3]={0};
	INT8U year[5]={0};
	INT16U i,j,k;
	INT8U Months[12][6]={"Jan01","Feb02","Mar03","Apr04","May05","Jun06","Jul07","Aug08","Sep09","Oct10","Nov11","Dec12"};
	
	
	temp0 = temp;
#if DBG_ON == 1
	print_string("BUILD TIME: "__DATE__"-"__TIME__"\r\n" );
	print_string("data:%s\r\n", temp );
	print_string("data0:%s\r\n", temp0 );
#endif
	for(i=0;i<3;i++)
		month[i] = *(temp0+i);
	
	for(i=0;i<2;i++)
		date[i] = *(temp0+4+i);
	
	for(i=0;i<4;i++)
		year[i] = *(temp0+7+i);
		
#if DBG_ON == 1
	print_string("month = %s\r\n", month );
	print_string("date = %s\r\n", date );
	print_string("year = %s\r\n", year );
#endif

	for(j=0;j<12;j++)
	{
//		gp_strncmp();
		for(i=0;i<3;i++)
		{
			if(month[i] == Months[j][i])
			{
				if(i == 2)
					break;
			}
			else
			{
				i = 0;
				break;
			}
			
		}
		if(i == 2)
		{
			for(k=0;k<2;k++)
			{
				month[k] = Months[j][i+1+k];	
			}
//			print_string("month = %s\r\n", month );
			break;
		}
	}
	
	for(i=0;i<4;i++)
		*(temp0+i) = year[i];
	for(i=0;i<2;i++)
		*(temp0+4+i) = month[i];
	for(i=0;i<2;i++)
		*(temp0+6+i) = date[i];
	
		*(temp0+8) = ' ';

#if DBG_ON == 1
	print_string("data:%s\r\n", temp );
	print_string("data0:%s\r\n", temp0 );
#endif
	
	return (temp0);
}
