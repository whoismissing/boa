#include "cluster_common.h"

unsigned char debug_level=0;

unsigned short get_checksum(unsigned char *ptr, unsigned short len)
{
    int i;
    unsigned int csum=0;

    for(i=0; i<len-1; i+=2)
    {
        csum += (ptr[i]<<8);
        csum += ptr[i+1];
    }
    if(i == (len-1))
        csum += ptr[i];

    csum=(csum>>16)+(csum&0xFFFF);  
    csum+=(csum >> 16);  
    return (unsigned short)(~csum);
}

void get_debug_level()
{
	if(isFileExist(CLUSTER_DEBUG_LEVEL_FILE))
	{
		FILE *fp = fopen(CLUSTER_DEBUG_LEVEL_FILE, "r");
		if(fp!=NULL)
		{
			unsigned char level = 0x0;
			fread(&level, 1, 1, fp);
			level -= '0';
			if(level!=debug_level){
				printf("update cluster debug level to %d\n", level);
				debug_level = (level==0)?0:1;
			}
			fclose(fp);
		}
	}
}

