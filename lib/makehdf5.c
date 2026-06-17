#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <zip.h>
#include <hdf5_hl.h>

#include "plexostables.h"
#include "makehdf5.h"

#define H5PLEXOS_VERSION "v0.6.0"

const char* object_names[n_membershipfields] = {
    [first] = "name", [second] = "category"
};

const char* relation_names[n_membershipfields] = {
    [first] = "parent", [second] = "child"
};

size_t field_offsets[n_membershipfields] = {
    [first] = HOFFSET(struct plexosMembershipRow, first),
    [second] = HOFFSET(struct plexosMembershipRow, second)
};

struct debugTraceConfig {
    bool initialized;
    bool enabled;
    const char* collection;
    const char* property;
    const char* object;
    size_t max_rows;
    size_t value_count;
    size_t emitted_rows;
};

static struct debugTraceConfig trace_config = {0};

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

static size_t parse_env_size_t(const char* env_name, size_t default_value) {

    const char* raw = getenv(env_name);
    if (raw == NULL || raw[0] == '\0') {
        return default_value;
    }

    char* endptr = NULL;
    long parsed = strtol(raw, &endptr, 10);
    if (endptr == raw || *endptr != '\0' || parsed < 0) {
        fprintf(stderr,
                "Warning: ignoring invalid %s='%s'; using %zu\n",
                env_name,
                raw,
                default_value);
        return default_value;
    }

    return (size_t)parsed;

}

static void init_trace_config(void) {

    if (trace_config.initialized) {
        return;
    }

    trace_config.initialized = true;
    trace_config.collection = getenv("H5PLEXOS_TRACE_COLLECTION");
    trace_config.property = getenv("H5PLEXOS_TRACE_PROPERTY");
    trace_config.object = getenv("H5PLEXOS_TRACE_OBJECT");

    bool explicit_enable = env_truthy(getenv("H5PLEXOS_TRACE"));
    bool has_filter = (trace_config.collection != NULL && trace_config.collection[0] != '\0')
        || (trace_config.property != NULL && trace_config.property[0] != '\0')
        || (trace_config.object != NULL && trace_config.object[0] != '\0');

    trace_config.enabled = explicit_enable || has_filter;
    trace_config.max_rows = parse_env_size_t("H5PLEXOS_TRACE_MAX_ROWS", 20);
    trace_config.value_count = parse_env_size_t("H5PLEXOS_TRACE_VALUE_COUNT", 24);

    if (trace_config.enabled) {
        fprintf(stderr,
                "Debug trace enabled: collection='%s' property='%s' object='%s' max_rows=%zu value_count=%zu\n",
                trace_config.collection == NULL ? "" : trace_config.collection,
                trace_config.property == NULL ? "" : trace_config.property,
                trace_config.object == NULL ? "" : trace_config.object,
                trace_config.max_rows,
                trace_config.value_count);
    }

}

static bool matches_filter(const char* filter, const char* value) {

    if (filter == NULL || filter[0] == '\0') {
        return true;
    }

    if (value == NULL) {
        return false;
    }

    return strcmp(filter, value) == 0;

}

static bool object_matches_filter(struct plexosMembership* membership, const char* object_filter) {

    if (object_filter == NULL || object_filter[0] == '\0') {
        return true;
    }

    if (membership == NULL || membership->collection.ptr == NULL) {
        return false;
    }

    if (membership->collection.ptr->isobjects) {
        if (membership->childobject.ptr == NULL) {
            return false;
        }
        return strcmp(object_filter, membership->childobject.ptr->name) == 0;
    }

    bool parent_match = membership->parentobject.ptr != NULL
        && strcmp(object_filter, membership->parentobject.ptr->name) == 0;
    bool child_match = membership->childobject.ptr != NULL
        && strcmp(object_filter, membership->childobject.ptr->name) == 0;

    return parent_match || child_match;

}

static void trace_values_preview(
    const char* stage,
    struct plexosKeyIndex* ki,
    const double* values,
    size_t n_values,
    size_t n_preview) {

    size_t preview_count = n_values < n_preview ? n_values : n_preview;

    fprintf(stderr,
            "Debug trace values %s: key_idx=%zu preview_count=%zu total_length=%zu\n",
            stage,
            ki->key.idx,
            preview_count,
            n_values);

    for (size_t i = 0; i < preview_count; i++) {
        fprintf(stderr,
                "Debug trace value %s: key_idx=%zu local_idx=%zu value=%.17g\n",
                stage,
                ki->key.idx,
                i,
                values[i]);
    }

}

static void debug_scan_nonfinite_values(
    const char* stage,
    struct plexosKeyIndex* ki,
    struct plexosKey* key,
    const double* values,
    size_t n_values) {

    size_t inf_count = 0;
    size_t nan_count = 0;
    size_t first_idx = 0;
    bool has_nonfinite = false;

    for (size_t i = 0; i < n_values; i++) {
        if (isinf(values[i])) {
            inf_count++;
            if (!has_nonfinite) {
                first_idx = i;
                has_nonfinite = true;
            }
        } else if (isnan(values[i])) {
            nan_count++;
            if (!has_nonfinite) {
                first_idx = i;
                has_nonfinite = true;
            }
        }
    }

    if (has_nonfinite) {
        const char* collection_name = key->membership.ptr->collection.ptr->h5name;
        const char* property_name = key->property.ptr->name;
        fprintf(stderr,
                "Debug non-finite %s: periodtype=%d phase=%d band=%d key_idx=%zu position=%ld length=%d collection=%s property=%s inf=%zu nan=%zu first_local_idx=%zu first_value=%g\n",
                stage,
                ki->periodtype,
                key->phase,
                key->band,
                ki->key.idx,
                ki->position,
                ki->length,
                collection_name,
                property_name,
                inf_count,
                nan_count,
                first_idx,
                values[first_idx]);
    }

}

void add_configs(hid_t f) {

    H5LTset_attribute_string(f, "/", "h5plexos", H5PLEXOS_VERSION);

    struct plexosConfig** rows = *(tables[config].rows);
    size_t n_rows = tables[config].count;

    for (size_t i = 0; i < n_rows; i++) {
        H5LTset_attribute_string(f, "/", rows[i]->element, rows[i]->value);
    }

}

void add_collections(hid_t meta, int compressionlevel) {

    size_t n_collections = tables[collection].count;
    size_t n_memberships = tables[membership].count;

    for (size_t m = 0; m < n_memberships; m++) {

        struct plexosMembership* membership = data.memberships[m];
        struct plexosCollection* collection = membership->collection.ptr;
        size_t c_m = membership->collection_membership_idx;

        if (collection->rows == NULL) {
            collection->rows = calloc(collection->nmembers, sizeof(struct plexosMembershipRow));
        }
        struct plexosMembershipRow* row = &(collection->rows[c_m]);

        if (collection->isobjects) {
            strcpy(row->first, membership->childobject.ptr->name);
            strcpy(row->second, membership->childobject.ptr->category.ptr->name);
        } else {
            strcpy(row->first, membership->parentobject.ptr->name);
            strcpy(row->second, membership->childobject.ptr->name);
        }

    }

    hid_t objects =
        H5Gcreate2(meta, "objects", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    hid_t relations =
        H5Gcreate2(meta, "relations", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    hid_t string_type = H5Tcopy(H5T_C_S1);
    H5Tset_size(string_type, MAXSTRINGLENGTH);

    hid_t field_types[n_membershipfields];
    field_types[first] = string_type;
    field_types[second] = string_type;

    hid_t table_group;
    const char** field_names;

    for (size_t c = 0; c < n_collections; c++) {

        struct plexosCollection* collection = data.collections[c];

        if (collection->nmembers > 0) {

            if (collection->isobjects) {
                table_group = objects;
                field_names = object_names;
            } else {
                table_group = relations;
                field_names = relation_names;
            }

            // This sets the maximum dataspace size to infinite (the current
            // size is of course table.nrows). Not a big deal, but might be
            // nice to keep current == max

            // This "table" construct also adds a bunch of unneccesary metadata
            // about the compound datatype. Would be preferable to just define
            // a compound (string,string) struct from scratch.

            H5TBmake_table(
                collection->h5name, table_group, collection->h5name,
                n_membershipfields, collection->nmembers, sizeof(struct plexosMembershipRow),
                field_names, field_offsets, field_types, collection->nmembers,
                NULL, compressionlevel, collection->rows);

        }

    }

    H5Gclose(objects);
    H5Gclose(relations);
    H5Tclose(string_type);

}

void add_times(hid_t meta, const char* localformat, int compressionlevel) {

    // PLEXOS data format notes:
    // Period type 0 - phase-native interval / block data
    // Maps to ST periodtype 0 via relevant phase table
    // Store both block and interval results on disk (if not ST)?

    // Period type 1-7 - period-type-specific data
    // Direct mapping to period labels

    hid_t times =
        H5Gcreate2(meta, "times", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    hid_t string_type = H5Tcopy(H5T_C_S1);
    H5Tset_size(string_type, 20);


    for (size_t i_t = 0; i_t < n_plexosperiods; i_t++) {

        struct plexosTable table = tables[period_tables[i_t]];

        if (!table.count) continue;

        void** periodrows = *(table.rows);
        char* period_timestamps = calloc(table.count, sizeof(char[20]));
        memset(period_timestamps,'\0', table.count * sizeof(char[20]));

        for (size_t i = 0; i < table.count; i++) {
            table.puttimestamp(period_timestamps + i*sizeof(char[20]),
                               periodrows[i], localformat);
        }

        H5LTmake_dataset(times, table.h5name, 1, &table.count,
                         string_type, period_timestamps);

    }

    H5Gclose(times);

}

void set_attribute_int(hid_t dset, char* name, size_t* value) {
    hid_t dspace = H5Screate(H5S_SCALAR);
    hid_t attr = H5Acreate(dset, name, H5T_NATIVE_INT, dspace, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attr, H5T_NATIVE_INT, value);
    H5Aclose(attr);
    H5Sclose(dspace);
}

hid_t dataset(hid_t dat, struct plexosKeyIndex* ki, int compressionlevel) {

    struct plexosTable* phase = get_phasetype(ki->key.ptr->phase);
    struct plexosTable* period = get_periodtype(ki->periodtype);
    struct plexosCollection* collection = ki->key.ptr->membership.ptr->collection.ptr;
    struct plexosProperty* property = ki->key.ptr->property.ptr;

    bool is_summarydata = property->issummary && ki->periodtype != 0;
    char* property_name = is_summarydata ? property->summaryname : property->name;

    hid_t h5phase = H5LTpath_valid(dat, phase->h5name, true) ?
        H5Gopen2(dat, phase->h5name, H5P_DEFAULT) :
        H5Gcreate2(dat, phase->h5name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (h5phase < 0) {
        fprintf(stderr, "Error getting HDF5 phase group '%s'", phase->h5name);
        exit(EXIT_FAILURE);
    }

    hid_t h5period = H5LTpath_valid(h5phase, period->h5name, true) ?
        H5Gopen2(h5phase, period->h5name, H5P_DEFAULT) :
        H5Gcreate2(h5phase, period->h5name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (h5period < 0) {
        fprintf(stderr, "Error getting HDF5 period group '%s'", period->h5name);
        exit(EXIT_FAILURE);
    }

    hid_t h5coll = H5LTpath_valid(h5period, collection->h5name, true) ?
        H5Gopen2(h5period, collection->h5name, H5P_DEFAULT) :
        H5Gcreate2(h5period, collection->h5name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (h5coll < 0) {
        fprintf(stderr, "Error getting HDF5 collection group '%s'", collection->h5name);
        exit(EXIT_FAILURE);
    }

    hid_t dset;
    if (H5LTpath_valid(h5coll, property_name, true)) {

        dset = H5Dopen2(h5coll, property_name, H5P_DEFAULT);

    } else {

        hsize_t dims[3] = {collection->nmembers, ki->length, property->nbands};
        hsize_t chunk_dims[3] = {1, ki->length, property->nbands};

        hid_t dset_space = H5Screate_simple(3, dims, NULL);

        hid_t dset_properties = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_deflate(dset_properties, compressionlevel);
        H5Pset_chunk(dset_properties, 3, chunk_dims);

        dset = H5Dcreate2(h5coll, property_name, H5T_IEEE_F64LE, dset_space,
                          H5P_DEFAULT, dset_properties, H5P_DEFAULT);
        set_attribute_int(dset, "period_offset", &(ki->periodoffset));
        H5LTset_attribute_string(h5coll, property_name, "units",
            is_summarydata ? property->summaryunit.ptr->value : property->unit.ptr->value);

        H5Sclose(dset_space);
        H5Pclose(dset_properties);

    }
    if (dset < 0) {
        fprintf(stderr, "Error getting HDF5 property dataset '%s'", property_name);
        exit(EXIT_FAILURE);
    }

    H5Gclose(h5phase);
    H5Gclose(h5period);
    H5Gclose(h5coll);

    return dset;

}

void add_values(hid_t dat, int compressionlevel) {

    init_trace_config();

    for (size_t i = 0; i < tables[key_index].count; i++) {

        struct plexosKeyIndex* ki = data.keyindices[i];
        struct plexosKey* key = ki->key.ptr;

        hid_t dset = dataset(dat, ki, compressionlevel);
        hsize_t start[3] = {key->membership.ptr->collection_membership_idx, 0, key->band-1};
        hsize_t data_dims[3] = {1, ki->length, 1};

        hid_t source_space = H5Screate_simple(3, data_dims, NULL);
        hid_t dest_space = H5Dget_space(dset);
        H5Sselect_hyperslab(dest_space, H5S_SELECT_SET, start, NULL, data_dims, NULL);

        double* values = &(data.values[ki->periodtype][ki->position / sizeof(double)]);
        const char* collection_name = key->membership.ptr->collection.ptr->h5name;
        bool is_summarydata = key->property.ptr->issummary && ki->periodtype != 0;
        const char* property_name = is_summarydata ?
            key->property.ptr->summaryname : key->property.ptr->name;
        bool should_trace = trace_config.enabled
            && trace_config.emitted_rows < trace_config.max_rows
            && matches_filter(trace_config.collection, collection_name)
            && matches_filter(trace_config.property, property_name)
            && object_matches_filter(key->membership.ptr, trace_config.object);

        if (should_trace) {
            const char* parent_name = key->membership.ptr->parentobject.ptr == NULL ?
                "" : key->membership.ptr->parentobject.ptr->name;
            const char* child_name = key->membership.ptr->childobject.ptr == NULL ?
                "" : key->membership.ptr->childobject.ptr->name;
            fprintf(stderr,
                    "Debug trace row: key_idx=%zu collection=%s property=%s phase=%d periodtype=%d band=%d position=%ld length=%d member_row=%llu parent=%s child=%s\n",
                    ki->key.idx,
                    collection_name,
                    property_name,
                    key->phase,
                    ki->periodtype,
                    key->band,
                    ki->position,
                    ki->length,
                    (unsigned long long)start[0],
                    parent_name,
                    child_name);
            trace_values_preview("pre-write", ki, values, (size_t)ki->length, trace_config.value_count);
        }

        debug_scan_nonfinite_values("pre-write", ki, key, values, ki->length);

        herr_t write_err =
            H5Dwrite(dset, H5T_NATIVE_DOUBLE, source_space, dest_space, H5P_DEFAULT, values);
        if (write_err < 0) {
            fprintf(stderr,
                    "Error writing dataset values: key_idx=%zu periodtype=%d position=%ld length=%d\n",
                    ki->key.idx, ki->periodtype, ki->position, ki->length);
            exit(EXIT_FAILURE);
        }

        double* verify_values = calloc((size_t)ki->length, sizeof(double));
        if (verify_values != NULL) {
            herr_t read_err =
                H5Dread(dset, H5T_NATIVE_DOUBLE, source_space, dest_space, H5P_DEFAULT, verify_values);
            if (read_err < 0) {
                fprintf(stderr,
                        "Error reading dataset values after write: key_idx=%zu periodtype=%d position=%ld length=%d\n",
                        ki->key.idx, ki->periodtype, ki->position, ki->length);
                exit(EXIT_FAILURE);
            }
            debug_scan_nonfinite_values("post-write", ki, key, verify_values, ki->length);
            if (should_trace) {
                trace_values_preview("post-write", ki, verify_values, (size_t)ki->length, trace_config.value_count);
                trace_config.emitted_rows++;
            }
            free(verify_values);
        } else {
            fprintf(stderr,
                    "Warning: could not allocate verification buffer for key_idx=%zu length=%d\n",
                    ki->key.idx, ki->length);
        }

        H5Sclose(source_space);
        H5Sclose(dest_space);
        H5Dclose(dset);

    }

}

char default_localformat[20] = "%d/%m/%Y %T";

void create_hdf5(zip_t* archive, int* err, const char* outfile) {

    // TODO: set these as runtime arguments
    int compressionlevel = 1;
    char* timeformat = &default_localformat;

    hid_t f = H5Fcreate(outfile, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    add_configs(f);

    hid_t meta =
        H5Gcreate2(f, "metadata", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    add_collections(meta, compressionlevel);
    add_times(meta, timeformat, compressionlevel);
    H5Gclose(meta);

    hid_t dat =
        H5Gcreate2(f, "data", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    add_values(dat, compressionlevel);
    H5Gclose(dat);

    H5Fclose(f);

}
