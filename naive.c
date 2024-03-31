#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BSIZE 1<<10

struct result {
    char city[100];
    int count;
    double sum, min, max;
};

int get_city(const char *city, struct result results[], int nresults) {
    for (int i = 0; i < nresults; i++) {
        if (strcmp(results[i].city, city) == 0) {
            return i;
        }
    }

    return -1;
}

int cmp(const void *ptr_a, const void *ptr_b) {
    return strcmp(((struct result *)ptr_a)->city, ((struct result *)ptr_b)->city);
}

int main(int argc, const char **argv) {
    const char *file;
    if (argc == 2) {
        file = argv[1];
    } else {
        printf("use ./main.out <file>\n");
        exit(EXIT_FAILURE);
    }
    
    FILE *fptr;
    
    fptr = fopen(file, "r");
    char buffer[BSIZE];
    
    struct result results[450];
    int nresults = 0;

    while(fgets(buffer, BSIZE, fptr)) {
        char *pos = strchr(buffer, ';');
        *pos = 0x0;
        
        int c = get_city(buffer, results, nresults);
        double curr = strtof(pos+1, NULL);

        if(c < 0) {
            strcpy(results[nresults].city, buffer);
            results[nresults].min = curr;
            results[nresults].max = curr;
            results[nresults].sum = curr;
            results[nresults].count = 1;
            nresults++;
        } else {
            if(curr < results[c].min) {
                results[c].min = curr;
            }
            if(curr > results[c].max) {
                results[c].max = curr;
            }
            results[c].sum += curr;
            results[c].count += 1;
        }

    }
    fclose(fptr); 

    qsort(results, (size_t)nresults, sizeof(*results), cmp);

    putchar('{');
    for(int i=0; i<nresults; i++) {
        printf("%s=%.1f/%.1f/%.1f%s", results[i].city, results[i].min, 
                results[i].max, results[i].sum/results[i].count, 
                i < nresults-1 ? ", " : "");
    }
    puts("}");
}
