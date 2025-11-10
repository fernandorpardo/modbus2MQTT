/** ************************************************************************************************
 *	C-strings management library
 *  (c) Fernando R (iambobot.com)
 *
 *  C strings management
 *
 *  1.0.0 - May 2021
 *  1.1.0 - October 2022
 *  1.2.0 - November 2022
 *  1.2.1 - January 2023
 *  1.2.2 - November 2024
 *
 ** ************************************************************************************************
**/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include "cstr.h"

/** 
* -------------------------------------------------------------------------------------------------
*
*      CSTR - Character string management functions
*
* -------------------------------------------------------------------------------------------------
**/
// search 'needle' into 'str' and
// retuns
//    the postion in 'needle' if found
//    -1 if not found
int cstr_find(char* str, char const* needle, int pos, int max_sz)
{
	int i= 0;
	int j= 0;
	if(str==0 || needle==0 || needle[0]=='\0') return -1;
	if(pos>0) while(str[i]!='\0' && i<pos) i++;
	if(str[i]=='\0') return -1;
	while(str[i]!='\0' && (max_sz==0 || i<max_sz))
	{
		char *p=&str[i], c= needle[0];
		for(; *p!=c && (max_sz==0 || i<max_sz) && str[i]!='\0'; p++, i++);
		if(*p==c)
		{
			j=0;
			char *p= &str[i+j], *pn= (char*)&needle[j];
			for(; *p==*pn && (max_sz==0 || (i+j)<max_sz) && *p!='\0'; p++, pn++, j++);
			if(needle[j]=='\0' && j>0) 
				return i;
			i++;
		}
	}
	return -1;
} // cstr_find

void cstr_dump(char * bf, int nbytes)
{
	char str[32];
	str[0]='\0';
	int j=0;
	for(int i=0; i<nbytes; i++)
	{
		char c= bf[i];
		fprintf(stdout, "%02X ", bf[i]);
		if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>=0x20 && c<=0x7e)) str[j++]= c;
		else str[j++]='.';
		str[j]='\0';
		if(j== 16)
		{
			fprintf(stdout,"   %s\n", str);
			j=0;
		}
	}
	if(j)
	{
		while(j++<16) fprintf(stdout,"   ");
		fprintf(stdout,"   %s\n", str);
	}
}

void cstr_fdump(FILE *fp, char * bf, int nbytes)
{
	char str[32];
	str[0]='\0';
	int j=0;
	char sbf[64];
	for(int i=0; i<nbytes; i++)
	{
		char c= bf[i];
		snprintf(sbf, sizeof(sbf), "%02X ", bf[i]);
		fwrite(sbf,  strlen(sbf), 1, fp);
		if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>=0x20 && c<=0x7e)) str[j++]= c;
		else str[j++]='.';
		str[j]='\0';
		if(j== 16)
		{
			snprintf(sbf, sizeof(sbf),"   %s\n", str);
			fwrite(sbf,  strlen(sbf), 1, fp);
			j=0;
		}
	}
	if(j)
	{
		while(j++<16) {
			snprintf(sbf, sizeof(sbf),"   ");
			fwrite(sbf,  strlen(sbf), 1, fp);
		}
		
		snprintf(sbf, sizeof(sbf),"   %s\n", str);
		fwrite(sbf,  strlen(sbf), 1, fp);
	}
}

void cstr_replace (char *str, char a, char b)
{
	if(str==0 || str[0]=='\0') return;
	for(int i=0; str[i]!='\0'; ) 
		if(str[i] == a) 
		{
			// remove
			if(b=='\0') for(int j=i; str[j]!='\0'; j++) str[j]= str[j+1];
			// replace
			else str[i++]=b;
		}
		else i++;
} // cstr_replace()

char *cstr_sub (char *str, char *sub, int p0, int p1)
{
	int i=0;
	for(; (p0+i)<=p1 && str[p0+i]!='\0'; i++) sub[i]= str[p0+i];
	sub[i]='\0';
	return sub;
} // cstr_sub()

// to be used instead of snprintf to prevent destination size warning
char* cstr_copy(char* dst, char* org , size_t dst_max_sz)
{
	int i=0;
	int sz= (int) dst_max_sz;
	for(; org[i]!='\0' && i<(sz - 1); i++) dst[i]=org[i];
	dst[i]= '\0';
	return dst;
} // cstr_copy


// END OF FILE