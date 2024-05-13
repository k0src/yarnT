#include <stdio.h>

int main() {
    printf("Hello, this is an example C program!\n");

    // Declare and initialize variables
    int a = 5;
    int b = 3;
    int sum;

    // Perform arithmetic operation
    sum = a + b;

    // Display the result
    printf("The sum of %d and %d is: %d\n", a, b, sum);

    // Check if sum is even or odd
    if (sum % 2 == 0) {
        printf("The sum is even.\n");
    } else {
        printf("The sum is odd.\n");
    }

    return 0;
}