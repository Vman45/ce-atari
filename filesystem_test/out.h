#ifndef _OUT_H_
#define _OUT_H_

void out_sw(char *str1, WORD w1);
void out_sc(char *str1, char c);

void initBuffer(void);
void appendToBuf(char *str);
void writeBufferToFile(void);
void deinitBuffer(void);

#endif