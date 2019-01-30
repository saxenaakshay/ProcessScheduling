#include <stdio.h>
#include <math.h>
#define RAND_MAX 077777

double pow(double base, int exponent){
	if(exponent < 0){
		return pow((1/base), exponent);
	}	
	int i;
	double ans = 1;
	for(i = 1;i <= exponent; i++){
		ans = base * ans;
	}
	return ans;
}

/* this works for 0<y<2 (which is the case for expdev */
double log(double num){
    double result = 0;
    double multiplier = (num-1);
    int i;
    for(i = 1;i <= 20;i++){
        result += (((i%2==0) ? -1:1) * multiplier / i);
        multiplier *= (num-1);
    }
    return result;
}

int expdev(double lambda){
	double dummy;
	do{
		dummy = (double)rand() / RAND_MAX;
	}while(dummy == 0.0);
    double ans = -log(dummy) / lambda;
    return (int)ans;
}