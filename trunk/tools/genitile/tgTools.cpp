#include "string.h"
#include "stdlib.h"

///////////////////////////////////////////////////////////////////////////////
// replace a car
char* replace_car(char *buf,char src,char dst)
{
	char *temp = buf;

    while(temp=strrchr(temp,src))
    {	*temp = dst;
	}
	return buf;
}

///////////////////////////////////////////////////////////////////////////////
//

int strpos(char *buf,char c)
{	for(int i=0;i<strlen(buf);i++)
	{	if(buf[i]==c)
			return i;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// substring

void substring(char *dst,char *buf,int start,int length)
{
	int l=strlen(buf);
	if(start+length<=l && l>0)
	{	if(length==0)
		{	length=l-start;}
		int i=0;
		for(i=0;i<length;i++)
		{	dst[i]=buf[start+i];
		}
		dst[i]=0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// remove a char

void remove_car(char *buf,char c)
{
	int pos=strpos(buf,c);
	while(pos!=-1)
	{
		char l[256]={""};

		if(pos+1<strlen(buf))
			substring(l,buf,pos+1,0);
		buf[pos]=0;
		strcat(buf,l);

		pos=strpos(buf,c);
	}
}
