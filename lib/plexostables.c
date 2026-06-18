#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

// using strptime, so won't compile on Windows without gcc/mingw
#include <time.h>

#include "plexostables.h"

#define STDTIMEFORMAT "%FT%T"

struct tm temptime = {};

bool strequals(const char* x, const char* y) {
    return strcmp(x, y) == 0;
}

bool strbool(const char* x) {
    return strequals(x, "true");
}

char* stralloc(const char* x) {
    char* p = calloc(strlen(x) + 1, sizeof(char));
    strcpy(p, x);
    return p;
}

void strcat_nospaces(char* dest, char* src) {
    for (size_t i = 0; i < strlen(src); i++) {
        if (src[i] != ' ') strncat(dest, src + i, 1);
    }
}

void* idx2ptr(size_t idx, int table_idx) {

    struct plexosTable table = tables[table_idx];
    void* p = NULL;

    if (idx <= table.max_idx) {
        p = (*(table.rows))[idx];
    }

    if (idx > table.max_idx || p == NULL) {
        //fprintf(stderr, "%s index %u not available\n", table.name, idx);
    }

    return p;

}


void link_noop(void* p) {}

void populate_config(void* p, const char* field, const char* value) {

    struct plexosConfig* row = p;

    if (strequals(field, "element")) {
        row->element = stralloc(value);
    } else if (strequals(field, "value")) {
        row->value = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in config table\n", field);
        exit(EXIT_FAILURE);
    }

}

void populate_unit(void* p, const char* field, const char* value) {

    struct plexosUnit* row = p;

    if (strequals(field, "value")) {
        row->value = stralloc(value);
    } else if (strequals(field, "lang_id")) {
        row->lang = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in unit table\n", field);
        exit(EXIT_FAILURE);
    }
}

void populate_timeslice(void* p, const char* field, const char* value) {

    struct plexosTimeslice* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in timeslice table\n", field);
        exit(EXIT_FAILURE);
    }
}

void populate_model(void* p, const char* field, const char* value) {

    struct plexosModel* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in model table\n", field);
        exit(EXIT_FAILURE);
    }
}

// This function won't be called (assuming well-formed XML)
void populate_band(void* p, const char* field, const char* value) {

    fprintf(stderr, "Unexpected field %s in band table\n", field);
    exit(EXIT_FAILURE);
}

void populate_sample(void* p, const char* field, const char* value) {

    struct plexosSample* row = p;

    if (strequals(field, "sample_name")) {
        row->name = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in sample table\n", field);
        exit(EXIT_FAILURE);
    }
}

void populate_sample_weight(void* p, const char* field, const char* value) {

    struct plexosSampleWeight* row = p;

    if (strequals(field, "phase_id")) {
        row->phase = atoi(value);
    } else if (strequals(field, "value")) {
        row->value = atof(value);
    } else if (strequals(field, "sample_id")) {
        row->sample.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in sample_weight table\n", field);
        exit(EXIT_FAILURE);
    }
}

void link_sample_weight(void* p) {
    struct plexosSampleWeight* row = p;
    row->sample.ptr = idx2ptr(row->sample.idx, sample);
}

void populate_class_group(void* p, const char* field, const char* value) {

    struct plexosClassGroup* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else if (strequals(field, "lang_id")) {
        row->lang = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in class_group table\n", field);
        exit(EXIT_FAILURE);
    }
}

void populate_class(void* p, const char* field, const char* value) {

    struct plexosClass* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else if (strequals(field, "state")) {
        row->state = atoi(value);
    } else if (strequals(field, "lang_id")) {
        row->lang = atoi(value);
    } else if (strequals(field, "class_group_id")) {
        row->classgroup.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in class table\n", field);
        exit(EXIT_FAILURE);
    }
}

void link_class(void* p) {
    struct plexosClass* row = p;
    row->classgroup.ptr = idx2ptr(row->classgroup.idx, class_group);
}

void populate_category(void* p, const char* field, const char* value) {

    struct plexosCategory* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else if (strequals(field, "rank")) {
        row->rank = atoi(value);
    } else if (strequals(field, "class_id")) {
        row->class.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in category table\n", field);
        exit(EXIT_FAILURE);
    }
}

void link_category(void* p) {
    struct plexosCategory* row = p;
    row->class.ptr = idx2ptr(row->class.idx, class);
}

void populate_attribute(void* p, const char* field, const char* value) {

    struct plexosAttribute* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else if (strequals(field, "description")) {
        row->description = stralloc(value);
    } else if (strequals(field, "enum_id")) {
        row->enum_ = atoi(value);
    } else if (strequals(field, "class_id")) {
        row->class.idx = atoi(value);
    } else if (strequals(field, "lang_id")) {
        row->lang = atoi(value);
    } else if (strequals(field, "input_mask")) {
        // ignore input_mask fields
    } else {
        fprintf(stderr, "Unexpected field %s in attribute table\n", field);
        exit(EXIT_FAILURE);
    }
}

void link_attribute(void* p) {
    struct plexosAttribute* row = p;
    row->class.ptr = idx2ptr(row->class.idx, class);
}

void populate_collection(void* p, const char* field, const char* value) {

    struct plexosCollection* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else if (strequals(field, "complement_name")) {
        row->complementname = stralloc(value);
    } else if (strequals(field, "lang_id")) {
        row->lang = atoi(value);
    } else if (strequals(field, "parent_class_id")) {
        row->parentclass.idx = atoi(value);
    } else if (strequals(field, "child_class_id")) {
        row->childclass.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in collection table\n", field);
        exit(EXIT_FAILURE);
    }
}

void link_collection(void* p) {

    struct plexosCollection* collection = p;

    collection->parentclass.ptr = idx2ptr(collection->parentclass.idx, class);
    collection->childclass.ptr = idx2ptr(collection->childclass.idx, class);

    if (collection->parentclass.ptr == NULL || collection->childclass.ptr == NULL) {
        return; // this collection is not of practical interest
    }

    collection->isobjects = strcmp(collection->parentclass.ptr->name, "System") == 0;
    collection->h5name[0] = '\0';

    size_t namesize = 0;

    if (!collection->isobjects) {

        char* prefix = strcmp(collection->complementname, "") == 0 ?
            collection->parentclass.ptr->name : collection->complementname;

        namesize = strlen(prefix);

        if (namesize + 1 <= MAXSTRINGLENGTH) {
            strcat_nospaces(collection->h5name, prefix);
            strcat(collection->h5name, "_");
        } else {
            // This is not strictly true, since we're counting the spaces
            // against the length but not actually storing them
            fprintf(stderr, "Encountered a collection prefix longer than %d characters: %s\n",
                    MAXSTRINGLENGTH, prefix);
            exit(EXIT_FAILURE);
        }
    }

    namesize += strlen(collection->name);
    if (namesize <= MAXSTRINGLENGTH) {
        strcat_nospaces(collection->h5name, collection->name);
    } else {
        // This is not strictly true, since we're counting the spaces against
        // the the length but not actually storing them
        fprintf(stderr, "Encountered a collection name longer than %d characters: %s%s\n",
            MAXSTRINGLENGTH, collection->h5name, collection->name);
        exit(EXIT_FAILURE);
    }

    // TODO: Force ASCII characters only

    for (size_t i = 0; collection->h5name[i] != '\0'; i++) {
        collection->h5name[i] = tolower(collection->h5name[i]);
    }

}

void populate_property(void* p, const char* field, const char* value) {

    struct plexosProperty* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else if (strequals(field, "summary_name")) {
        row->summaryname = stralloc(value);
    } else if (strequals(field, "lang_id")) {
        row->lang = atoi(value);
    } else if (strequals(field, "enum_id")) {
        row->enum_ = atoi(value);
    } else if (strequals(field, "is_multi_band")) {
        row->ismultiband = strbool(value);
    } else if (strequals(field, "is_period")) {
        row->isperiod = strbool(value);
    } else if (strequals(field, "is_summary")) {
        row->issummary = strbool(value);
    } else if (strequals(field, "unit_id")) {
        row->unit.idx = atoi(value);
    } else if (strequals(field, "summary_unit_id")) {
        row->summaryunit.idx = atoi(value);
    } else if (strequals(field, "collection_id")) {
        row->collection.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in property table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_property(void* p) {
    struct plexosProperty* row = p;
    row->unit.ptr = idx2ptr(row->unit.idx, unit);
    row->summaryunit.ptr = idx2ptr(row->summaryunit.idx, unit);
    row->collection.ptr = idx2ptr(row->collection.idx, collection);
}

void populate_object(void* p, const char* field, const char* value) {

    struct plexosObject* row = p;

    if (strequals(field, "name")) {
        row->name = stralloc(value);
    } else if (strequals(field, "index")) {
        row->index = atoi(value);
    } else if (strequals(field, "show")) {
        row->show = strbool(value);
    } else if (strequals(field, "class_id")) {
        row->class.idx = atoi(value);
    } else if (strequals(field, "category_id")) {
        row->category.idx = atoi(value);
    } else if (strequals(field, "GUID")) {
        // ignore guid fields
    } else {
        fprintf(stderr, "Unexpected field %s in object table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_object(void* p) {
    struct plexosObject* row = p;
    row->class.ptr = idx2ptr(row->class.idx, class);
    row->category.ptr = idx2ptr(row->category.idx, category);
}

void populate_membership(void* p, const char* field, const char* value) {

    struct plexosMembership* row = p;

    if (strequals(field, "parent_class_id")) {
        row->parentclass.idx = atoi(value);
    } else if (strequals(field, "child_class_id")) {
        row->childclass.idx = atoi(value);
    } else if (strequals(field, "collection_id")) {
        row->collection.idx = atoi(value);
    } else if (strequals(field, "parent_object_id")) {
        row->parentobject.idx = atoi(value);
    } else if (strequals(field, "child_object_id")) {
        row->childobject.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in membership table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_membership(void* p) {

    struct plexosMembership* membership = p;

    membership->parentclass.ptr = idx2ptr(membership->parentclass.idx, class);
    membership->childclass.ptr = idx2ptr(membership->childclass.idx, class);
    membership->collection.ptr = idx2ptr(membership->collection.idx, collection);
    membership->parentobject.ptr = idx2ptr(membership->parentobject.idx, object);
    membership->childobject.ptr = idx2ptr(membership->childobject.idx, object);

    struct plexosCollection* coll = membership->collection.ptr;
    membership->collection_membership_idx = coll->nmembers;
    coll->nmembers++;

}

void populate_attribute_data(void* p, const char* field, const char* value) {

    struct plexosAttributeData* row = p;

    if (strequals(field, "value")) {
        row->value = atof(value);
    } else if (strequals(field, "attribute_id")) {
        row->attribute.idx = atoi(value);
    } else if (strequals(field, "object_id")) {
        row->object.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in object table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_attribute_data(void* p) {
    struct plexosAttributeData* row = p;
    row->attribute.ptr = idx2ptr(row->attribute.idx, attribute);
    row->object.ptr = idx2ptr(row->object.idx, object);
}

void populate_period_0(void* p, const char* field, const char* value) {

    struct plexosPeriod0* row = p;

    if (strequals(field, "period_of_day")) {
        row->periodofday = atoi(value);
    } else if (strequals(field, "hour_id")) {
        row->hour = atoi(value);
    } else if (strequals(field, "day_id")) {
        row->day = atoi(value);
    } else if (strequals(field, "week_id")) {
        row->week = atoi(value);
    } else if (strequals(field, "month_id")) {
        row->month = atoi(value);
    } else if (strequals(field, "quarter_id")) {
        row->quarter = atoi(value);
    } else if (strequals(field, "fiscal_year_id")) {
        row->fiscalyear = atoi(value);
    } else if (strequals(field, "datetime")) {
        row->datetime = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in period_0 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void puttimestamp_period_0(char* timestamp, void* p, const char* localformat) {
    struct plexosPeriod0* row = p;
    strptime(row->datetime, localformat, &temptime);
    strftime(timestamp, 20, STDTIMEFORMAT, &temptime);
}

void populate_period_1(void* p, const char* field, const char* value) {

    struct plexosPeriod1* row = p;

    if (strequals(field, "week_id")) {
        row->week = atoi(value);
    } else if (strequals(field, "month_id")) {
        row->month = atoi(value);
    } else if (strequals(field, "quarter_id")) {
        row->quarter = atoi(value);
    } else if (strequals(field, "fiscal_year_id")) {
        row->fiscalyear = atoi(value);
    } else if (strequals(field, "date")) {
        row->date = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in period_1 table\n", field);
        exit(EXIT_FAILURE);
    }

}
void puttimestamp_period_1(char* timestamp, void* p, const char* localformat) {
    struct plexosPeriod1* row = p;
    strncat(timestamp, row->date, 20);
}

void populate_period_2(void* p, const char* field, const char* value) {

    struct plexosPeriod2* row = p;

    if (strequals(field, "week_ending")) {
        row->weekending = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in period_2 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void puttimestamp_period_2(char* timestamp, void* p, const char* localtimestamp) {
    struct plexosPeriod2* row = p;
    strncat(timestamp, row->weekending, 20);
}

void populate_period_3(void* p, const char* field, const char* value) {

    struct plexosPeriod3* row = p;

    if (strequals(field, "month_beginning")) {
        row->monthbeginning = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in period_3 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void puttimestamp_period_3(char* timestamp, void* p, const char* localtimestamp) {
    struct plexosPeriod3* row = p;
    strncat(timestamp, row->monthbeginning, 20);
}

void populate_period_4(void* p, const char* field, const char* value) {

    struct plexosPeriod4* row = p;

    if (strequals(field, "year_ending")) {
        row->yearending = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in period_4 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void puttimestamp_period_4(char* timestamp, void* p, const char* localtimestamp) {
    struct plexosPeriod4* row = p;
    strncat(timestamp, row->yearending, 20);
}

void populate_period_6(void* p, const char* field, const char* value) {

    struct plexosPeriod6* row = p;

    if (strequals(field, "day_id")) {
        row->day = atoi(value);
    } else if (strequals(field, "datetime")) {;
        row->datetime = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in period_6 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void puttimestamp_period_6(char* timestamp, void* p, const char* localformat) {
    struct plexosPeriod6* row = p;
    strptime(row->datetime, localformat, &temptime);
    strftime(timestamp, 20, STDTIMEFORMAT, &temptime);
}

void populate_period_7(void* p, const char* field, const char* value) {

    struct plexosPeriod7* row = p;

    if (strequals(field, "quarter_beginning")) {
        row->quarterbeginning = stralloc(value);
    } else {
        fprintf(stderr, "Unexpected field %s in period_7 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void puttimestamp_period_7(char* timestamp, void* p, const char* localtimestamp) {
    struct plexosPeriod7* row = p;
    strncat(timestamp, row->quarterbeginning, 20);
}

void populate_phase_2(void* p, const char* field, const char* value) {

    struct plexosPhase2* row = p;

    if (strequals(field, "period_id")) {
        row->period = atoi(value);
    } else if (strequals(field, "interval_id")) {;
        row->interval.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in phase_2 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_phase_2(void* p) {
    struct plexosPhase2* row = p;
    row->interval.ptr = idx2ptr(row->interval.idx, period_0);
}

void populate_phase_3(void* p, const char* field, const char* value) {

    struct plexosPhase3* row = p;

    if (strequals(field, "period_id")) {
        row->period = atoi(value);
    } else if (strequals(field, "interval_id")) {;
        row->interval.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in phase_3 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_phase_3(void* p) {
    struct plexosPhase3* row = p;
    row->interval.ptr = idx2ptr(row->interval.idx, period_0);
}

void populate_phase_4(void* p, const char* field, const char* value) {

    struct plexosPhase4* row = p;

    if (strequals(field, "period_id")) {
        row->period = atoi(value);
    } else if (strequals(field, "interval_id")) {;
        row->interval.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in phase_4 table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_phase_4(void* p) {
    struct plexosPhase4* row = p;
    row->interval.ptr = idx2ptr(row->interval.idx, period_0);
}

void populate_key(void* p, const char* field, const char* value) {

    struct plexosKey* row = p;

    if (strequals(field, "phase_id")) {
        row->phase = atoi(value);
    } else if (strequals(field, "period_type_id")) {
        row->periodtype = atoi(value);
    } else if (strequals(field, "band_id")) {
        row->band = atoi(value);
    } else if (strequals(field, "membership_id")) {;
        row->membership.idx = atoi(value);
    } else if (strequals(field, "model_id")) {;
        row->model.idx = atoi(value);
    } else if (strequals(field, "property_id")) {;
        row->property.idx = atoi(value);
    } else if (strequals(field, "sample_id")) {;
        row->sample.idx = atoi(value);
    } else if (strequals(field, "timeslice_id")) {;
        row->timeslice.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in key table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_key(void* p) {

    struct plexosKey* row = p;
    row->membership.ptr = idx2ptr(row->membership.idx, membership);
    row->model.ptr = idx2ptr(row->model.idx, model);
    row->property.ptr = idx2ptr(row->property.idx, property);
    row->sample.ptr = idx2ptr(row->sample.idx, sample);
    row->timeslice.ptr = idx2ptr(row->timeslice.idx, timeslice);

    struct plexosProperty* prop = row->property.ptr;
    if (row->band > prop->nbands) {
        prop->nbands = row->band;
    }

}

void populate_key_index(void* p, const char* field, const char* value) {

    struct plexosKeyIndex* row = p;

    if (strequals(field, "period_type_id")) {
        row->periodtype = atoi(value);
    } else if (strequals(field, "position")) {
        row->position = atol(value);
    } else if (strequals(field, "length")) {
        row->length = atoi(value);
    } else if (strequals(field, "period_offset")) {
        row->periodoffset = atoi(value);
    } else if (strequals(field, "key_id")) {;
        row->key.idx = atoi(value);
    } else {
        fprintf(stderr, "Unexpected field %s in key_index table\n", field);
        exit(EXIT_FAILURE);
    }

}

void link_key_index(void* p) {
    struct plexosKeyIndex* row = p;
    row->key.ptr = idx2ptr(row->key.idx, key);
}

int period_tables[n_plexosperiods] = {
    [_period_0] = period_0, [_period_1] = period_1, [_period_2] = period_2, 
    [_period_3] = period_3, [_period_4] = period_4, [_period_6] = period_6, 
    [_period_7] = period_7
};

struct plexosData data = {0};

struct plexosTable tables[n_plexostables] = {

    [config] = {.name = "t_config",
                .populator = populate_config,
                .linker = link_noop,
                .rows = (void***) &data.configs,
                .rowsize = sizeof(struct plexosConfig)},

    [unit] = {.name = "t_unit",
              .id = "unit_id",
              .populator = populate_unit,
              .linker = link_noop,
              .rows = (void***) &data.units,
              .rowsize = sizeof(struct plexosUnit)},

    [timeslice] = {.name = "t_timeslice",
                   .id = "timeslice_id",
                   .populator = populate_timeslice,
                   .linker = link_noop,
                   .rows = (void***) &data.timeslices,
                   .rowsize = sizeof(struct plexosTimeslice)},

    [model] = {.name = "t_model",
               .id = "model_id",
               .populator = populate_model,
               .linker = link_noop,
               .rows = (void***) &data.models,
               .rowsize = sizeof(struct plexosModel)},

    [band] = {.name = "t_band",
              .id = "band_id",
              .populator = populate_band,
              .linker = link_noop,
              .rows = (void***) &data.bands,
              .rowsize = sizeof(struct plexosBand)},

    [sample] = {.name = "t_sample",
                .id = "sample_id",
                .linker = link_noop,
                .populator = populate_sample,
                .rows = (void***) &data.samples,
                .rowsize = sizeof(struct plexosSample)},

    [sample_weight] = {.name = "t_sample_weight",
                       .populator = populate_sample_weight,
                       .linker = link_sample_weight,
                       .rows = (void***) &data.sampleweights,
                       .rowsize = sizeof(struct plexosSampleWeight)},

    [class_group] = {.name= "t_class_group",
                     .id = "class_group_id",
                     .populator = populate_class_group,
                     .linker = link_noop,
                     .rows = (void***) &data.classgroups,
                     .rowsize = sizeof(struct plexosClassGroup)},

    [class] = {.name = "t_class",
               .id = "class_id",
               .populator = populate_class,
               .linker = link_class,
               .rows = (void***) &data.classes,
               .rowsize = sizeof(struct plexosClass)},

    [category] = {.name = "t_category",
                  .id = "category_id",
                  .populator = populate_category,
                  .linker = link_category,
                  .rows = (void***) &data.categories,
                  .rowsize = sizeof(struct plexosCategory)},

    [attribute] = {.name = "t_attribute",
                   .id = "attribute_id",
                   .populator = populate_attribute,
                   .linker = link_attribute,
                   .rows = (void***) &data.attributes,
                   .rowsize = sizeof(struct plexosAttribute)},

    [collection] = {.name = "t_collection",
                    .id = "collection_id",
                    .populator = populate_collection,
                    .linker = link_collection,
                    .rows = (void***) &data.collections,
                    .rowsize = sizeof(struct plexosCollection)},

    [property] = {.name = "t_property",
                  .id = "property_id",
                  .populator = populate_property,
                  .linker = link_property,
                  .rows = (void***) &data.properties,
                  .rowsize = sizeof(struct plexosProperty)},

    [object] = {.name = "t_object",
                .id = "object_id",
                .populator = populate_object,
                .linker = link_object,
                .rows = (void***) &data.objects,
                .rowsize = sizeof(struct plexosObject)},

    [membership] = {.name = "t_membership",
                    .id = "membership_id",
                    .populator = populate_membership,
                    .linker = link_membership,
                    .rows = (void***) &data.memberships,
                    .rowsize = sizeof(struct plexosMembership)},

    [attribute_data] = {.name = "t_attribute_data",
                        .populator = populate_attribute_data,
                        .linker = link_attribute_data,
                        .rows = (void***) &data.attributedata,
                        .rowsize = sizeof(struct plexosAttributeData)},

    [period_0] = {.name = "t_period_0",
                  .id = "interval_id",
                  .h5name = "interval",
                  .populator = populate_period_0,
                  .linker = link_noop,
                  .puttimestamp = puttimestamp_period_0,
                  .rows = (void***) &data.intervals,
                  .rowsize = sizeof(struct plexosPeriod0)},

    [period_1] = {.name = "t_period_1",
                  .id = "day_id",
                  .h5name = "day",
                  .populator = populate_period_1,
                  .linker = link_noop,
                  .puttimestamp = puttimestamp_period_1,
                  .rows = (void***) &data.days,
                  .rowsize = sizeof(struct plexosPeriod1)},

    [period_2] = {.name = "t_period_2",
                  .id = "week_id",
                  .h5name = "week",
                  .populator = populate_period_2,
                  .linker = link_noop,
                  .puttimestamp = puttimestamp_period_2,
                  .rows = (void***) &data.weeks,
                  .rowsize = sizeof(struct plexosPeriod2)},

    [period_3] = {.name = "t_period_3",
                  .id = "month_id",
                  .h5name = "month",
                  .populator = populate_period_3,
                  .linker = link_noop,
                  .puttimestamp = puttimestamp_period_3,
                  .rows = (void***) &data.months,
                  .rowsize = sizeof(struct plexosPeriod3)},

    [period_4] = {.name = "t_period_4",
                  .id = "fiscal_year_id",
                  .h5name = "year",
                  .populator = populate_period_4,
                  .linker = link_noop,
                  .puttimestamp = puttimestamp_period_4,
                  .rows = (void***) &data.years,
                  .rowsize = sizeof(struct plexosPeriod4)},

    [period_6] = {.name = "t_period_6",
                  .id = "hour_id",
                  .h5name = "hour",
                  .populator = populate_period_6,
                  .linker = link_noop,
                  .puttimestamp = puttimestamp_period_6,
                  .rows = (void***) &data.hours,
                  .rowsize = sizeof(struct plexosPeriod6)},

    [period_7] = {.name = "t_period_7",
                  .id = "quarter_id",
                  .h5name = "quarter",
                  .populator = populate_period_7,
                  .linker = link_noop,
                  .puttimestamp = puttimestamp_period_7,
                  .rows = (void***) &data.quarters,
                  .rowsize = sizeof(struct plexosPeriod7)},

    [phase_2] = {.name = "t_phase_2",
                 .h5name = "PASA",
                 .populator = populate_phase_2,
                 .linker = link_phase_2,
                 .rows = (void***) &data.pasa,
                 .rowsize = sizeof(struct plexosPhase2)},

    [phase_3] = {.name = "t_phase_3",
                 .h5name = "MT",
                 .populator = populate_phase_3,
                 .linker = link_phase_3,
                 .rows = (void***) &data.mt,
                 .rowsize = sizeof(struct plexosPhase3)},

    [phase_4] = {.name = "t_phase_4",
                 .h5name = "ST",
                 .populator = populate_phase_4,
                 .linker = link_phase_4,
                 .rows = (void***) &data.st,
                 .rowsize = sizeof(struct plexosPhase4)},

    [key] = {.name = "t_key",
             .id = "key_id",
             .populator = populate_key,
             .linker = link_key,
             .rows = (void***) &data.keys,
             .rowsize = sizeof(struct plexosKey)},

    [key_index] = {.name = "t_key_index",
                   .populator = populate_key_index,
                   .linker = link_key_index,
                   .rows = (void***) &data.keyindices,
                   .rowsize = sizeof(struct plexosKeyIndex)}

};

/* TODO: Switch to a more performant data structure for lookups (trie?) */
struct plexosTable* get_plexostable(const char* tablename) {

    struct plexosTable* ptr = NULL;

    for (size_t i = 0; i < n_plexostables; i++) {
        if (strcmp(tablename, tables[i].name) == 0) {
            ptr = &(tables[i]);
            break;
        }
    }

    return ptr;

}

struct plexosTable* get_phasetype(size_t phase) {
    switch (phase) {
        case 2: return &tables[phase_2];
        case 3: return &tables[phase_3];
        case 4: return &tables[phase_4];
        default: fprintf(stderr, "Phase %zu not recognized", phase);
                 exit(EXIT_FAILURE);
    };
}

struct plexosTable* get_periodtype(size_t period) {
    switch (period) {
        case 0: return &tables[period_0];
        case 1: return &tables[period_1];
        case 2: return &tables[period_2];
        case 3: return &tables[period_3];
        case 4: return &tables[period_4];
        case 6: return &tables[period_6];
        case 7: return &tables[period_7];
        default: fprintf(stderr, "Period %zu not recognized", period);
                 exit(EXIT_FAILURE);
    };
}

void* row_array(int table_idx) {
    size_t n = tables[table_idx].max_idx + 1;
    void* p = calloc(n, sizeof(void*));
    memset(p, 0, n * sizeof(void*));
    return p;
}

void init_data() {

    data.configs = row_array(config);
    data.units = row_array(unit);
    data.timeslices = row_array(timeslice);
    data.models = row_array(model);
    data.bands = row_array(band);

    data.samples = row_array(sample);
    data.sampleweights = row_array(sample_weight);

    data.classgroups = row_array(class_group);
    data.classes = row_array(class);
    data.categories = row_array(category);

    data.attributes = row_array(attribute);
    data.collections = row_array(collection);
    data.properties = row_array(property);

    data.objects = row_array(object);
    data.memberships = row_array(membership);
    data.attributedata = row_array(attribute_data);

    data.intervals = row_array(period_0);
    data.days = row_array(period_1);
    data.weeks = row_array(period_2);
    data.months = row_array(period_3);
    data.years = row_array(period_4);
    data.hours = row_array(period_6);
    data.quarters = row_array(period_7);

    data.pasa = row_array(phase_2);
    data.mt = row_array(phase_3);
    data.st = row_array(phase_4);

    data.keys = row_array(key);
    data.keyindices = row_array(key_index);

}

void finalize_data() {

    // Link up data structures
    for (size_t t = 0; t < n_plexostables; t++) {
        void** rows = *(tables[t].rows);
        for (size_t i = 0; i <= tables[t].max_idx; i++) {
            if (rows[i] != NULL) {
                tables[t].linker(rows[i]);
            }
        }
    }

    // Consolidate data arrays
    for (size_t t = 0; t < n_plexostables; t++) {
        void** rows = *(tables[t].rows);
        size_t next_dense = 0;
        for (size_t i = 0; i <= tables[t].max_idx; i++) {
            if (rows[i] != NULL) {
                rows[next_dense] = rows[i];
                next_dense++;
            }
        }
    }

}
