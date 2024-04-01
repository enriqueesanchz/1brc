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

static const char *hash(int *len, unsigned int *h, const char* str)
{
    for (; *(str+*len)!=';'; (*len)++) {
        *h = *h * 31 + *(str+*len);
    }
    *h = *h & (HCAP-1);
    return str+*len+1;
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
    char buf[1024 * 16];
    buf[0] = '{';
    char *b = buf+1;
    for (int i = 0; i < nresults; i++) {
        b += sprintf(b, "%s=%.1f/%.1f/%.1f%s", results[i].city, results[i].min,
                 results[i].sum / results[i].count, results[i].max,
                 i < nresults - 1 ? ", " : "}");
    }
    *b = 0x0;
    puts(buf);
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
            char *city = pos;
    
            int h = 0;
            int delim = 0;
            hash(&delim, &h, city);
            int c = map[h];
            while (c != -1 && memcmp(results[c].city, city, (size_t)delim) != 0) {
                h = (h + 1) & (HCAP - 1);
                c = map[h];
            }

            double curr;
            pos = parse_pointer(&curr, pos+delim+1);

            if(c < 0) {
                memcpy(results[nresults].city, city, (size_t)delim);
                results[nresults].city[delim] = '\0';
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
