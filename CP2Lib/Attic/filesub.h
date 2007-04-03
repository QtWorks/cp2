#pragma once

void   set(char *str, char *fmt, void *parm, char *keyword, int line, char *fname);
int    parse(char *keyword,char *parms[]);
void   getkeyval(char *line,char *keyword,char *value);
