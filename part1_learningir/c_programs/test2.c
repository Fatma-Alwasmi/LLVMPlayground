#include <stdio.h>

int main(void){
	int a, b, d;

	a = getchar();
	b = 1;
	d = 0;

	if (a == 0){
		a = (b >= 0);
		d = b/a;
	}
		return 0;
}
