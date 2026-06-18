#ifndef plexostables_h_INCLUDED
#define plexostables_h_INCLUDED

#include <stdbool.h>

#define MAXSTRINGLENGTH 255 // TODO: Generalize this with the XML length

enum plexosTableIdx {
    config, unit, timeslice, model, band, sample, sample_weight,
    class_group, class, category, attribute, collection, property,
    object, membership, attribute_data,
    period_0, period_1, period_2, period_3, period_4, period_6, period_7,
    phase_2, phase_3, phase_4, key, key_index, n_plexostables
};

enum plexosPeriodIdx {
    _period_0, _period_1, _period_2, _period_3, _period_4, _period_6, _period_7,
    n_plexosperiods
};

extern int period_tables[n_plexosperiods];

typedef void populate_function(void* row, const char* field, const char* value);
typedef void link_function(void* row);
typedef void timestamp_function(char* timestamp, void* row, const char* localformat);

struct plexosTable {

    char* name;
    char* id;
    char* h5name;

    size_t count;
    int max_idx;

    size_t nextidx;
    size_t rowsize;

    populate_function* populator;
    link_function* linker;
    timestamp_function* puttimestamp;

    void*** rows;

};

extern struct plexosTable tables[n_plexostables];

struct plexosTable* get_plexostable(const char* tablename);

struct plexosTable* get_phasetype(size_t phase);
struct plexosTable* get_periodtype(size_t phase);

// PLEXOS table row data

struct plexosConfig {
    char* element;
    char* value;
};

union plexosConfigRef {
    struct plexosConfig* ptr;
    size_t idx;
};


struct plexosUnit {
    char* value;
    int lang;
};

union plexosUnitRef {
    struct plexosUnit* ptr;
    size_t idx;
};


struct plexosTimeslice {
    char* name;
};

union plexosTimesliceRef {
    struct plexosTimeslice* ptr;
    size_t idx;
};


struct plexosModel {
    char* name;
};

union plexosModelRef {
    struct plexosModel* ptr;
    size_t idx;
};


struct plexosBand {
};

union plexosBandRef {
    struct plexosBand* ptr;
    size_t idx;
};


struct plexosSample {
    char* name; // sample_name
};

union plexosSampleRef {
    struct plexosSample* ptr;
    size_t idx;
};


struct plexosSampleWeight {
    int phase; // phase_id
    double value;
    union plexosSampleRef sample; // sample_id
};

union plexosSampleWeightRef {
    struct plexosSampleWeight* ptr;
    size_t idx;
};


struct plexosClassGroup {
    char* name;
    int lang;
};

union plexosClassGroupRef {
    struct plexosClassGroup* ptr;
    size_t idx;
};


struct plexosClass {
    char* name;
    int state;
    int lang;
    union plexosClassGroupRef classgroup; // class_group_id
};

union plexosClassRef {
    struct plexosClass* ptr;
    size_t idx;
};


struct plexosCategory {
    char* name;
    int rank;
    union plexosClassRef class; // class_id
};

union plexosCategoryRef {
    struct plexosCategory* ptr;
    size_t idx;
};


struct plexosAttribute {
    char* name;
    char* description;
    int enum_; // enum_id
    int lang;
    union plexosClassRef class; // class_id
};

union plexosAttributeRef {
    struct plexosAttribute* ptr;
    size_t idx;
};

enum membershipRowFields {
    first, second, n_membershipfields
};

struct plexosMembershipRow {
    char first[MAXSTRINGLENGTH+1];
    char second[MAXSTRINGLENGTH+1];
};

struct plexosCollection {
    char* name;
    char* complementname; // complement_name
    int lang;
    union plexosClassRef parentclass; // parent_class_id
    union plexosClassRef childclass; // child_class_id
    size_t nmembers;
    char h5name[MAXSTRINGLENGTH+1];
    bool isobjects;
    struct plexosMembershipRow* rows;
};

union plexosCollectionRef {
    struct plexosCollection* ptr;
    size_t idx;
};


struct plexosProperty {
    char* name;
    char* summaryname; // summary_name
    int lang;
    int enum_; // enum_id
    bool ismultiband; // is_multi_band
    bool isperiod; // is_period
    bool issummary; // is_summary
    union plexosUnitRef unit; // unit_id
    union plexosUnitRef summaryunit; // summary_unit_id
    union plexosCollectionRef collection; // collection_id
    size_t nbands;
};

union plexosPropertyRef {
    struct plexosProperty* ptr;
    size_t idx;
};


struct plexosObject {
    char* name;
    int index;
    bool show;
    union plexosClassRef class; // class_id
    union plexosCategoryRef category; // category_id
};

union plexosObjectRef {
    struct plexosObject* ptr;
    size_t idx;
};


struct plexosMembership {
    union plexosClassRef parentclass; // parent_class_id
    union plexosClassRef childclass; // child_class_id
    union plexosCollectionRef collection; // collection_id
    union plexosObjectRef parentobject; // parent_object_id
    union plexosObjectRef childobject; // child_object_id
    size_t collection_membership_idx;
};

union plexosMembershipRef {
    struct plexosMembership* ptr;
    size_t idx;
};


struct plexosAttributeData {
    double value;
    union plexosAttributeRef attribute; // attribute_id
    union plexosObjectRef object; // object_id
};

union plexosAttributeDataRef {
    struct plexosAttributeData* ptr;
    size_t idx;
};


struct plexosPeriod0 {
    int periodofday; // period_of_day
    int hour; // hour_id
    int day; // day_id
    int week; // week_id
    int month; // month_id
    int quarter; // quarter_id
    int fiscalyear; // fiscal_year_id
    char* datetime;
};

union plexosPeriod0Ref {
    struct plexosPeriod0* ptr;
    size_t idx;
};

struct plexosPeriod1 {
    int week; // week_id
    int month; // month_id
    int quarter; // quarter_id
    int fiscalyear; // fiscal_year_id
    char* date;
};

union plexosPeriod1Ref {
    struct plexosPeriod1* ptr;
    size_t idx;
};

struct plexosPeriod2 {
    char* weekending; // week_ending
};

union plexosPeriod2Ref {
    struct plexosPeriod2* ptr;
    size_t idx;
};

struct plexosPeriod3 {
    char* monthbeginning; // month_beginning
};

union plexosPeriod3Ref {
    struct plexosPeriod3* ptr;
    size_t idx;
};

struct plexosPeriod4 {
    char* yearending; // year_ending
};

union plexosPeriod4Ref {
    struct plexosPeriod4* ptr;
    size_t idx;
};

struct plexosPeriod6 {
    int day; // day_id
    char* datetime;
};

union plexosPeriod6Ref {
    struct plexosPeriod6* ptr;
    size_t idx;
};

struct plexosPeriod7 {
    char* quarterbeginning; // quarter_beginning
};

union plexosPeriod7Ref {
    struct plexosPeriod7* ptr;
    size_t idx;
};

struct plexosPhase2 {
    int period; // period_id
    union plexosPeriod0Ref interval; // interval_id
};

union plexosPhase2Ref {
    struct plexosPhase2* ptr;
    size_t idx;
};

struct plexosPhase3 {
    int period; // period_id
    union plexosPeriod0Ref interval; // interval_id
};

union plexosPhase3Ref {
    struct plexosPhase3* ptr;
    size_t idx;
};

struct plexosPhase4 {
    int period; // period_id
    union plexosPeriod0Ref interval; // interval_id
};

union plexosPhase4Ref {
    struct plexosPhase4* ptr;
    size_t idx;
};

struct plexosKey {
    int phase; // phase_id
    // note that this is not accurate, use KeyIndex.periodtype instead
    int periodtype; // period_type_id
    int band; // band_id
    union plexosMembershipRef membership; // membership_id
    union plexosModelRef model; // model_id
    union plexosPropertyRef property; // property_id
    union plexosSampleRef sample; // sample_id
    union plexosTimesliceRef timeslice; //timeslice_id
};

union plexosKeyRef {
    struct plexosKey* ptr;
    size_t idx;
};

struct plexosKeyIndex {
    int periodtype; // period_type_id
    long int position; // bytes from binary file start
    int length; // in 8-byte (64-bit float) increments
    int periodoffset; // period_offset // temporal data offset (if any) in stored times
    union plexosKeyRef key; // key_id
};

union plexosKeyIndexRef {
    struct plexosKeyIndex* ptr;
    size_t idx;
};

struct plexosData {

    struct plexosConfig** configs;
    struct plexosUnit** units;
    struct plexosTimeslice** timeslices;
    struct plexosModel** models;
    struct plexosBand** bands;

    struct plexosSample** samples;
    struct plexosSampleWeight** sampleweights;

    struct plexosClassGroup** classgroups;
    struct plexosClass** classes;
    struct plexosCategory** categories;

    struct plexosAttribute** attributes;
    struct plexosCollection** collections;
    struct plexosProperty** properties;

    struct plexosObject** objects;
    struct plexosMembership** memberships;
    struct plexosAttributeData** attributedata;

    struct plexosPeriod0** intervals;
    struct plexosPeriod1** days;
    struct plexosPeriod2** weeks;
    struct plexosPeriod3** months;
    struct plexosPeriod4** years;
    struct plexosPeriod6** hours;
    struct plexosPeriod7** quarters;

    struct plexosPhase2** pasa;
    struct plexosPhase3** mt;
    struct plexosPhase4** st;

    struct plexosKey** keys;
    struct plexosKeyIndex** keyindices;

    // TODO: Don't hardcode this?
    double* values[8];
};

extern struct plexosData data;

void init_data();
void finalize_data();

#endif // plexostables_h_INCLUDED
