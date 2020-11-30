#include <stdio.h>
#include <math.h>

void initVars(){	//This function will initialize the variables we'll use

uint a=24,b=6,c=8;	//Declaration of the integers a, b and c with their respective values
float result;		//Declaration of an empty float, that will be used later

}

void main(){

	initVars();		//Calling the function initVars so we can use our variables
	while(1){		//Start of a while loop, with an always true condition
		result=a/c;	//We asign the value of a divided by c to the variable result
		
		//Output "The result is: " followed by the float result with two decimals after floating point
		printf("The result is: %f.2\n",&result);
	}
}

