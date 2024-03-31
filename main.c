#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BSIZE (1<<20) * 64
#define SEED 0x12345678
#define HCAP 4096

struct result {
    char city[100];
    int count;
    double sum, min, max;
};

int cmp(const void *ptr_a, const void *ptr_b) {
    return strcmp(((struct result *)ptr_a)->city, ((struct result *)ptr_b)->city);
}

int hash(const char* str, int h)
{
    //MurmurOAAT_32 hash function
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

char *parse_pointer(double *d, char *pos) {
    double mod;
    if(*pos == '-') {
        mod = -1.0;
        pos++;
    } else {
        mod = 1.0;
    }

    if(*(pos+1) == '.') {
        *d = ((double)*pos + *(pos+2) * 0.1 - 1.1 * '0') * mod; //'0'==48
        return pos+4;
    } else {
        *d = (((double)*pos) * 10 + (double)*(pos+1) + ((double)*(pos+3)) * 0.1 - 11.1 * '0') * mod;
        return pos+5;
    }
}

void print_json(struct result *results, int nresults) {
    putchar('{');
    for(int i=0; i<nresults; i++) {
        printf("%s=%.1f/%.1f/%.1f%s", results[i].city, results[i].min, 
                results[i].max, results[i].sum/results[i].count, 
                i < nresults-1 ? ", " : "");
    }
    puts("}");
}

void main(int argc, const char **argv) {
    const char *file;
    if (argc == 2) {
        file = argv[1];
    } else {
        printf("use ./main.out <file>\n");
        exit(EXIT_FAILURE);
    }
    
    FILE *fptr;
    
    fptr = fopen(file, "r");
    
    struct result results[450];
    int nresults = 0;

    int map[HCAP]; // map of indexes
    memset(map, -1, HCAP*sizeof(int)); // index cannot be -1

    char *buffer = malloc(BSIZE);
    size_t ret;

    while(1) {
        ret = fread(buffer, sizeof(char), BSIZE, fptr);
        if(ret == 0) {
            break;
        }

        int rewind = 0;
        while (buffer[ret-1] != '\n') {
            rewind--;
            ret--;
        }
        fseek(fptr, rewind, SEEK_CUR);

        char *pos = buffer;
        while(pos < &buffer[ret]) {
            int delim = 0;
            while(*(pos+delim) != ';') {
                delim++;
            }
            *(pos+delim) = 0x0;
            char *city = pos;
    
            int h = hash(city, SEED) & (HCAP-1);
            int c = map[h];
            while (c != -1 && strcmp(results[c].city, city) != 0) {
                h = (h + 1) & (HCAP - 1);
                c = map[h];
            }

            double curr;
            pos = parse_pointer(&curr, pos+delim+1); // next to ; TODO: optimize with \n

            if(c < 0) {
                strcpy(results[nresults].city, city);
                results[nresults].min = curr;
                results[nresults].max = curr;
                results[nresults].sum = curr;
                results[nresults].count = 1;
                map[h] = nresults;
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
    }
    free(buffer);
    fclose(fptr); 

    qsort(results, (size_t)nresults, sizeof(*results), cmp);
    print_json(results, nresults);
}
