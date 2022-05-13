#include<stdio.h>

int c = 0;
int thirteenthrun = 0;
int inc(){
	c++;
	if (c==13 && thirteenthrun == 0){
		c--;
		thirteenthrun=1;
		return c;
	}
	if (c == 14 && thirteenthrun == 1){
		c++;
		thirteenthrun = 0;
		return c;
	}
	return c;
}

int reset(){
	c = 0;
	if (thirteenthrun==1) 
		thirteenthrun = 0;
	return c;
}
