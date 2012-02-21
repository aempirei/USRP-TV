#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	double a;
	double b=0;
	int n;

	while(1) {
		n=read(0, &a, sizeof(a));
		if(n==0)break;
		write(1, &a, sizeof(a));
		write(1, &a, sizeof(b));
	}
	exit(0);
}
