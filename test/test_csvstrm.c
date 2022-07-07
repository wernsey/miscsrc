#include <stdio.h>
#include <string.h>
#include <errno.h>

#define CSV_IMPLEMENTATION
#include "../csvstrm.h"

int main(int argc, char *argv[]) {
    CsvContext csv;
    FILE *f;

    if(argc < 2) {
        fprintf(stderr, "CSV file expected\n");
        return 1;
    }

    f = fopen(argv[1], "r");
    if(!f) {
        fprintf(stderr, "Unable to open '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

#if 1
    csv_context_file(&csv, f);
#else
    struct csv_read_limit ll;
    ll.f = f;
    ll.limit = 41;
    csv_context_file_limit(&csv, &ll);
#endif

    while(csv_read_record(&csv)) {
        int j;
        if(csv_get_error(&csv) != CSV_OK) {
            fprintf(stderr, "error: %d\n", csv_get_error(&csv));
            break;
        }
        for(j = 0; j < csv_count(&csv); j++) {
            printf("[%s]", csv_field(&csv,j));
        }
        printf("\n");
    }

    fclose(f);
    return 0;
}
