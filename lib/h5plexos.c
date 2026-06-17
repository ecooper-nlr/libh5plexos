#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <zip.h>

#include "plexostables.h"
#include "parsexml.h"
#include "makehdf5.h"

static bool env_truthy(const char* value) {
    if (value == NULL || value[0] == '\0') {
        return false;
    }
    char normalized[16];
    size_t i = 0;
    for (; value[i] != '\0' && i < sizeof(normalized) - 1; i++) {
        normalized[i] = (char)tolower((unsigned char)value[i]);
    }
    normalized[i] = '\0';
    return strcmp(normalized, "1") == 0
        || strcmp(normalized, "true") == 0
        || strcmp(normalized, "yes") == 0
        || strcmp(normalized, "on") == 0;
}

static void debug_scan_nonfinite_buffer(const char* label, const double* values, size_t n_values) {

    static int trace_nonfinite_once = 0;
    static bool trace_nonfinite = false;
    if (!trace_nonfinite_once) {
        trace_nonfinite = env_truthy(getenv("H5PLEXOS_TRACE_NONFINITE"));
        trace_nonfinite_once = 1;
    }

    if (!trace_nonfinite) {
        return;  // Skip nonfinite buffer tracing by default
    }

    size_t inf_count = 0;
    size_t nan_count = 0;
    size_t first_nonfinite_idx = 0;
    bool has_nonfinite = false;

    for (size_t i = 0; i < n_values; i++) {
        if (isinf(values[i])) {
            inf_count++;
            if (!has_nonfinite) {
                first_nonfinite_idx = i;
                has_nonfinite = true;
            }
        } else if (isnan(values[i])) {
            nan_count++;
            if (!has_nonfinite) {
                first_nonfinite_idx = i;
                has_nonfinite = true;
            }
        }
    }

    fprintf(stderr,
            "Debug non-finite scan [%s]: values=%zu inf=%zu nan=%zu\n",
            label, n_values, inf_count, nan_count);

    if (has_nonfinite) {
        fprintf(stderr,
                "Debug first non-finite [%s]: idx=%zu value=%g\n",
                label, first_nonfinite_idx, values[first_nonfinite_idx]);
    }

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
            zip_int64_t n = zip_fread(bin, data.values[i], stat.size);

            if (n < stat.size) {
                fprintf(stderr, "Only read %ld bytes from %lu byte file\n", n, stat.size);
                exit(EXIT_FAILURE);
            }

            if (stat.size % sizeof(double) != 0) {
                fprintf(stderr,
                        "Debug warning [%s]: size=%lu is not a multiple of %zu (double size)\n",
                        fname, stat.size, sizeof(double));
            }

            size_t n_values = stat.size / sizeof(double);
            debug_scan_nonfinite_buffer(fname, data.values[i], n_values);

        }

    }

    // Debug: Verify t_data_0.BIN was loaded with expected data
    if (data.values[0] != NULL) {
        // Check the specific offset we know should have 320 values (from Python verification)
        size_t offset_bytes = 1768048320;
        size_t offset_doubles = offset_bytes / sizeof(double);
        double* ptr = &(data.values[0][offset_doubles]);
        
        fprintf(stderr, "Debug: Verifying t_data_0.BIN at offset %zu bytes (double offset %zu)\n", 
                offset_bytes, offset_doubles);
        fprintf(stderr, "Debug: First 8 values at offset: %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",
                ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
    }

    finalize_data();
    create_hdf5(archive, &err, outfile);
    zip_discard(archive);

    return;

}
