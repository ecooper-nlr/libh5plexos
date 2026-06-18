#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zip.h>
#include <expat.h>

#include "plexostables.h"
#include "parsexml.h"

#define XMLBUFFERSIZE 32768
#define XMLMAXTEXTLENGTH 255

enum xmlDepth { root, row, rowData, n_xmllevels };

struct parserState {

    int depth;

    bool save_text;
    char text[XMLMAXTEXTLENGTH + 1];
    size_t text_length;

    struct plexosTable* table;
    void* row_data;

};

struct parserState state;

void reset_state() {
    memset(&state, 0, sizeof(struct parserState));
}

void parse(zip_t* archive, int* err, zip_int64_t xml_idx, struct parseSpec spec) {

    zip_file_t* xml = zip_fopen_index(archive, xml_idx, 0);
    if (xml == NULL) {
        fprintf(stderr, "Error %d occured when opening internal XML file.\n", *err);
        return;
    }

    XML_Parser p = XML_ParserCreate(NULL);
    if (p == NULL) {
        fprintf(stderr, "Error occured when allocating XML parser.\n");
        return;
    }
    XML_SetElementHandler(p, spec.start, spec.end);
    XML_SetCharacterDataHandler(p, spec.text);

    char buf[XMLBUFFERSIZE];
    bool done = false;
    int n;
    reset_state();

    while (!done) {

        n = zip_fread(xml, buf, XMLBUFFERSIZE);
        //printf("Read %d bytes\n", n);

        if (n <= 0) {
            done = true;
        }

        if (XML_Parse(p, buf, n, done) == XML_STATUS_ERROR) {
            fprintf(stderr,
                    "XML parse error at line %lu:\n%s\n",
                    XML_GetCurrentLineNumber(p), XML_ErrorString(XML_GetErrorCode(p)));
            return;
        }

    }
    XML_ParserFree(p);

    int ferr = zip_fclose(xml);
    if (ferr != 0) {
        fprintf(stderr, "Error %d occured when closing internal XML file.\n", ferr);
        return;
    }

}

// Summary (first pass) parsing

static void XMLCALL summary_start(void* data, const XML_Char* el, const XML_Char** attr) {
    (void)data;

    switch (state.depth) {

        case root:      if (strcmp(el, "SolutionDataset") != 0) {
                            fprintf(stderr, 
                                    "Unrecognized PLEXOS XML data format "
                                    "(root tag is not SolutionDataset)\n");
                            exit(EXIT_FAILURE);
                        }
                        break;

        case row:       state.table = get_plexostable(el);

                        if (state.table == NULL) {
                            fprintf(stderr, "Error parsing row in %s\n", el);
                            exit(EXIT_FAILURE);
                        }

                        if (state.table->count == 0) {
                            state.table->max_idx = -1;
                        }

                        state.table->count += 1;

                        if (state.table->id == NULL) {
                            state.table->max_idx += 1;
                        }

                        break;

        case rowData:   if (state.table->id != NULL) {
                            state.save_text =
                                (strcmp(el, state.table->id) == 0);
                        }
                        break;

        default:        fprintf(stderr,
                                "Unrecognized PLEXOS XML data format "
                                "(more than 3 levels of tags)\n");
                        exit(EXIT_FAILURE); 

    }

    state.depth++;

}

static void XMLCALL summary_text(void* data, const XML_Char* s, int n) {

    if (state.save_text) {

        state.text_length += n;

        if (state.text_length > XMLMAXTEXTLENGTH) {
            fprintf(stderr,
                    "Encountered a PLEXOS entity index longer than "
                    "%d characters: '%s%.*s'",
                    XMLMAXTEXTLENGTH, state.text, n, s);
            exit(EXIT_FAILURE);
        }

        strncat(state.text, s, n);

    }

}

static void XMLCALL summary_end(void* data, const XML_Char* el) {
    (void)data;
    (void)el;

    if (state.save_text) {

        int idx = atoi(state.text);

        if (idx > state.table->max_idx) {
            state.table->max_idx = idx;
        }

        state.save_text = false;
        state.text[0] = '\0';
        state.text_length = 0;

    }

    state.depth--;

}

struct parseSpec summary_pass = {
    .start = summary_start,
    .text = summary_text,
    .end = summary_end,
};


// Data (second pass) parsing

static void XMLCALL data_start(
    void* data, const XML_Char* el, const XML_Char** attr) {

    switch (state.depth) {

        case root:      // printf("%s\n", el);
                        if (strcmp(el, "SolutionDataset") != 0) {
                            fprintf(stderr, 
                                    "Unrecognized PLEXOS XML data format "
                                    "(root tag is not SolutionDataset)\n");
                            exit(EXIT_FAILURE);
                        }
                        break;

        case row:       // printf(" %s\n", el);
                        state.table = get_plexostable(el);
                        if (state.table == NULL) {
                            fprintf(stderr, "Error parsing row in %s\n", el);
                            exit(EXIT_FAILURE);
                        }
                        state.row_data = malloc(state.table->rowsize);
                        memset(state.row_data, 0, state.table->rowsize);
                        if (state.table->id == NULL) {
                            (*(state.table->rows))[state.table->nextidx] = state.row_data;
                            state.table->nextidx++;
                        }
                        break;

        case rowData:   state.save_text = true;
                        break;

        default:        fprintf(stderr,
                                "Unrecognized PLEXOS XML data format "
                                "(more than %d levels of tags)\n", n_xmllevels);
                        exit(EXIT_FAILURE); 

    }

    state.depth++;

}

static void XMLCALL data_text(void* data, const XML_Char* s, int n) {

    if (state.save_text) {

        state.text_length += n;

        if (state.text_length > XMLMAXTEXTLENGTH) {
            fprintf(stderr,
                    "Encountered a PLEXOS entity field value longer than "
                    "%d characters: '%s%.*s'",
                    XMLMAXTEXTLENGTH, state.text, n, s);
            exit(EXIT_FAILURE);
        }

        strncat(state.text, s, n);

    }

}

static void XMLCALL data_end(void* data, const XML_Char* el) {

    if (state.save_text) {

        // printf("  %s = %s\n", el, state.text);

        if ((state.table->id != NULL) && (strcmp(el, state.table->id) == 0)) {

            size_t idx = atoi(state.text);

            (*(state.table->rows))[idx] = state.row_data;

        } else {

            (state.table->populator)(
                state.row_data, el, state.text);

        }

        state.save_text = false;
        state.text[0] = '\0';
        state.text_length = 0;

    }

    state.depth--;

}

struct parseSpec populate_pass = {
    .start = data_start,
    .text = data_text,
    .end = data_end,
};
