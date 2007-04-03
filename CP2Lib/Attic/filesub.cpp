//#include <Afx.h>

#include        <stdio.h>
#include        <time.h>
#include        <string.h>
#include        <ctype.h>

void getkeyval(char *line,char *keyword,char *value)
   {
   int  i,j;
   
   for(i=0; line[i] && isspace(line[i]); i++);  /* move to non-whitespace */
   
   /* transfer nonwhitespaces from line to keyword */
   for(j=0; line[i] && !isspace(line[i]); i++,j++)  keyword[j] = line[i];
   keyword[j] = 0;   /* null terminate */
   
   for(; line[i] && isspace(line[i]); i++);  /* move to non-whitespace */
   
   /* transfer all remaining characters to value */
   /* ignoring comments delineated by ; */
   for(j=0; line[i] && line[i] != ';'; i++,j++)  value[j] = line[i];
   value[j] = 0;
   
   /* remove all whitespaces at the end of value[] */
   for(; j && (isspace(value[j]) || !value[j]); j--)   value[j] = 0;
   }


/* read in logic parameter with error catching */
/* fmt string is exactly like that for scanf except for:  */
/*   q   expects to scan for on or off or 1 or 0 */
void set(char *str, char *fmt, void *parm, char *keyword, int line, char *fname)
   {
   if(fmt[1] == 'q')   /* scan for on/off command */
      {
      if(!stricmp(str,"on"))         *(int *)parm = 1;
      else if(!stricmp(str,"1"))     *(int *)parm = 1;
      else if(!stricmp(str,"off"))   *(int *)parm = 0;
      else if(!stricmp(str,"0"))     *(int *)parm = 0;
      else if(!stricmp(str,"odd"))   *(int *)parm = 2;
      else if(!stricmp(str,"even"))  *(int *)parm = 4;
      else if(!stricmp(str,"both"))  *(int *)parm = 6;
      else 
	 {
	 printf("unrecognized q value \"%s\" for parameter \"%s\" in line %d of %s\n",str,keyword,line,fname);
	 *(int *)parm = 0;
	 return;
	 }
      }
   else /* then let sscanf do the work */
      {
      if(sscanf(str,fmt,parm) != 1)
	 {
	 printf("unrecognized %c value \"%s\" for parameter \"%s\" in line %d of %s\n",fmt[1],str,keyword,line,fname);
	 ((char *)parm)[0] = 0;
	 return;
	 }
      }
   }

int parse(char *keyword,char *parms[])
   {
   int  i;

   for(i=0; *parms[i]; i++)
      if(!stricmp(keyword,parms[i]))
	 break;

   return(i);
   }

void makefilename(char *pfilename, char *filename, char *path)
   {
   time_t       time_of_day;
   struct       tm      *fff;

   if(strlen(filename) == 0)    /* if filename not defined */
      {
      time_of_day = time(NULL);
      fff = gmtime(&time_of_day);
      sprintf(pfilename,"%s%s%02d%02d%02d%02d.%02dZ",path
           ,(strlen(path)>0 && path[strlen(path)-1] != '\\')?"\\":"",fff->tm_year%100,fff->tm_mon+1,
	   fff->tm_mday,fff->tm_hour,fff->tm_min);
      }
   else
      {
      /* insert the path in front of the filename */
      sprintf(pfilename,"%s%s%s",path,
	 (strlen(path)>0 && path[strlen(path)-1] != '\\')?"\\":"",filename);
      }
   }

FILE    *openoutputfile(char *filename, char *path)
   {
   char         fname[80];
   FILE         *fp;

   makefilename(fname,filename,path);
   
   fp = fopen(fname,"wb");

   if(!fp) {printf("cannot open data output file %s\n",fname); return NULL;}

   return(fp);
   }


