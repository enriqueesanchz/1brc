#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define BSIZE (1<<20) * 64
#define SEED 0x12345678
#define HCAP 4096
#define NTHREADS 4

struct result {
    char city[100];
    int count;
    double sum, min, max;
};

struct bundle {
    struct result results[450];
    int nresults;
    int map[HCAP];
};

int cmp(const void *ptr_a, const void *ptr_b) {
    return strcmp(((struct result *)ptr_a)->city, ((struct result *)ptr_b)->city);
}

int hash(int *len, const char *str)
{
    //TODO: improve saving len in result
    int h = 0;
    for (; *(str+*len)!='\0'; (*len)++) {
        h = h * 31 + *(str+*len);
    }
    h = h & (HCAP-1);
    return h;
}

static const char *parse_city(int *len, unsigned int *h, const char *str)
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

size_t fread_chunked(char *dest, FILE *fh) {
    static pthread_mutex_t lock;
    pthread_mutex_lock(&lock);
  
    size_t nread = fread(dest, 1, BSIZE, fh);
    if (nread <= 0) {
      pthread_mutex_unlock(&lock);
      return nread;
    }
  
    long rewind = 0;
    while (dest[nread - 1] != '\n') {
      rewind--;
      nread--;
    }
    fseek(fh, rewind, SEEK_CUR);
    pthread_mutex_unlock(&lock);
    return nread;
}

void *process_chunk(void *arg) {
    FILE *fh = (FILE *)arg;

    struct bundle *bundle = malloc(sizeof(*bundle));
    bundle->nresults = 0;

    memset(bundle->map, -1, HCAP*sizeof(int));

    char *buffer = malloc(BSIZE);

    while(1) {
        int ret = fread_chunked(buffer, fh);
        if(ret == 0) {
            break;
        }

        char *pos = buffer;
        while(pos < &buffer[ret]) {
            char *city = pos;
    
            int h = 0;
            int len = 0;
            parse_city(&len, &h, city);
            int c = bundle->map[h];
            while (c != -1 && memcmp(bundle->results[c].city, city, (size_t)len) != 0) {
                h = (h + 1) & (HCAP - 1);
                c = bundle->map[h];
            }

            double curr;
            pos = parse_pointer(&curr, pos+len+1);

            if(c < 0) {
                memcpy(bundle->results[bundle->nresults].city, city, (size_t)len);
                bundle->results[bundle->nresults].city[len] = '\0';
                bundle->results[bundle->nresults].min = curr;
                bundle->results[bundle->nresults].max = curr;
                bundle->results[bundle->nresults].sum = curr;
                bundle->results[bundle->nresults].count = 1;
                bundle->map[h] = bundle->nresults;
                bundle->nresults++;
            } else {
                if(curr < bundle->results[c].min) {
                    bundle->results[c].min = curr;
                }
                if(curr > bundle->results[c].max) {
                    bundle->results[c].max = curr;
                }
                bundle->results[c].sum += curr;
                bundle->results[c].count += 1;
            }
        }
    }
    free(buffer);
    return (void *)bundle;
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
    
    pthread_t workers[NTHREADS];
    for(int i=0; i<NTHREADS; i++) {
        pthread_create(&workers[i], NULL, process_chunk, fptr);
    }

    struct bundle *bundles[NTHREADS];
    for(int i=0; i<NTHREADS; i++) {
        pthread_join(workers[i], (void *)&bundles[i]);
    }

    for(int i=1; i<NTHREADS; i++) {
        for(int j=0; j<bundles[i]->nresults; j++) {
            int len = 0;
            int h = hash(&len, bundles[i]->results[j].city);
            int c = bundles[0]->map[h];
            while (c != -1 && memcmp(bundles[i]->results[j].city,
                        bundles[0]->results[c].city, (size_t)len) != 0) {
                h = (h + 1) & (HCAP - 1);
                c = bundles[0]->map[h];
            }

            if(bundles[0]->results[c].min > bundles[i]->results[j].min) {
                bundles[0]->results[c].min = bundles[i]->results[j].min;
            } 
            if(bundles[0]->results[c].max < bundles[i]->results[j].max) {
                bundles[0]->results[c].max = bundles[i]->results[j].max;
            }
            bundles[0]->results[c].sum += bundles[i]->results[j].sum;
            bundles[0]->results[c].count += bundles[i]->results[j].count;
        }
    }

    fclose(fptr); 

    qsort(bundles[0]->results, (size_t)bundles[0]->nresults, sizeof(*bundles[0]->results), cmp);
    print_json(bundles[0]->results, bundles[0]->nresults);
}
