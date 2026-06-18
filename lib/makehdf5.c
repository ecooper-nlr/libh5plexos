#include <stdlib.h>
#include <string.h>

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
        H5Dwrite(dset, H5T_NATIVE_DOUBLE, source_space, dest_space, H5P_DEFAULT, values);

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
