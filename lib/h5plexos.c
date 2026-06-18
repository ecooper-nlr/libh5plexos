#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <zip.h>

#include "plexostables.h"
#include "parsexml.h"
#include "makehdf5.h"

static zip_uint64_t read_zip_file_chunked(zip_file_t* bin, void* buffer, zip_uint64_t size) {

    const zip_uint64_t chunk_size = 64ULL * 1024ULL * 1024ULL;
    zip_uint64_t total_read = 0;
    unsigned char* out = buffer;

    while (total_read < size) {
        zip_uint64_t remaining = size - total_read;
        zip_uint64_t request = remaining < chunk_size ? remaining : chunk_size;
        zip_int64_t n = zip_fread(bin, out + total_read, request);

        if (n < 0) {
            return total_read;
        }

        if (n == 0) {
            break;
        }

        total_read += (zip_uint64_t)n;
    }

    return total_read;

}

void h5plexos(const char* infile, const char* outfile) {

    int err = 0;
    zip_t* archive = zip_open(infile, ZIP_RDONLY, &err);
    if (archive == NULL) {
        fprintf(stderr, "Error %d occured when loading zip file %s\n", err, infile);
        return;
    }

    const char* infile_name = strrchr(infile,  '/');
    infile_name = infile_name == NULL ? infile : &(infile_name[1]);
    size_t infile_length = strlen(infile_name);
    char xml_name[infile_length+1];
    strncpy(xml_name, infile_name, infile_length-3);
    xml_name[infile_length-3] = '\0';
    strcat(xml_name, "xml");
    printf("Looking for %s inside zip archive\n", xml_name);

    zip_int64_t xml_idx = zip_name_locate(archive, xml_name, 0);
    if (xml_idx == -1) {
        fprintf(stderr, "'%s' could not be found in the archive. "
                        "Are you sure this is a PLEXOS output?\n", xml_name);
        return;
    }

    parse(archive, &err, xml_idx, summary_pass);

    printf("Count\tMax Idx\tTable\n");
    printf("=====\t=======\t=====\n");
    for (int i = 0; i < n_plexostables; i++) {
        printf("%zu\t%d\t%s\n",
               tables[i].count, tables[i].max_idx, tables[i].name);
    }

    // libray invocations may init multiple times, reset_data instead?
    // also reset tables and parser state?
    init_data();
    parse(archive, &err, xml_idx, populate_pass);

    char fname[13];
    struct zip_stat stat = {};
    for (size_t i = 0; i < 8; i++) {

        sprintf(fname, "t_data_%zu.BIN", i);
        zip_int64_t bin_idx = zip_name_locate(archive, fname, 0);

        if (bin_idx >= 0) {

            zip_file_t* bin = zip_fopen_index(archive, bin_idx, 0);
            if (bin == NULL) {
                fprintf(stderr, "Error %d occured when opening %s.\n", err, fname);
                return;
            }

            zip_stat_index(archive, bin_idx, 0, &stat);
            printf("%s\t%lu bytes\n", fname, stat.size);
            data.values[i] = malloc(stat.size);
            if (data.values[i] == NULL) {
                fprintf(stderr, "Error: malloc(%lu) failed for %s\n", stat.size, fname);
                exit(EXIT_FAILURE);
            }

            zip_uint64_t n = read_zip_file_chunked(bin, data.values[i], stat.size);

            if (n < stat.size) {
                fprintf(stderr, "Only read %ld bytes from %lu byte file\n", n, stat.size);
                exit(EXIT_FAILURE);
            }

            zip_fclose(bin);

        }

    }

    finalize_data();
    create_hdf5(archive, &err, outfile);
    zip_discard(archive);

    return;

}
