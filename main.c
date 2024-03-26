#include <stdio.h>

#define FILENAME "measurements.txt"
#define BSIZE 1<<10

int main() {
    FILE *fptr;
    
    // Open a file in read mode
    fptr = fopen(FILENAME, "r");
    
    // Store the content of the file
    char buffer[BSIZE];
    
    // Read the content and print it
    while(fgets(buffer, BSIZE, fptr)) {
    }
    
    // Close the file
    fclose(fptr); 
}
