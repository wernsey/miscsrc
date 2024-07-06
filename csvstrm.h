#ifndef CSV_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * # CSV stream reader
 *
 * Single header library to read a [CSV file][wiki] row-by-row.
 *
 * To use this library, define `CSV_IMPLEMENTATION` before including
 * **csvstrm.h** in _one_ of your C files (other C files include
 * **csvstrm.h** normally), like so:
 *
 * ```
 * #include <stdio.h>
 *
 * #define CSV_IMPLEMENTATION
 * #include "csvstrm.h"
 * ```
 *
 * It will parse CSV documents as specified by [RFC4180][RFC], but it
 * follows the rule of _"be liberal in what you accept from others"_,
 * so there are a couple of deviations:
 *
 * * Leading and trailing whitespaces in each field are trimmed by default.
 *   * This behaviour can be changed by defining `CSV_TRIM` as 0.
 * * You can have spaces before and after the quotes in a quoted field.
 * * Double quotes inside unquoted fields are allowed.
 * * Records can end with CRLF or with LF character sequences.
 * * It does not enforce that all records (rows) have the same number of
 *   fields. That is an application concern.
 * * It does not specify whether the first row contains headers. That is
 *   left up to the application.
 *
 * It also took some ideas from the [Repici][] document cited by the [RFC][].
 *
 * ## Basic usage example
 *
 * Here is a simple usage example. Some error handling code has been omitted.
 *
 * ```
 * CsvContext csv;
 * FILE *f = fopen(argv[1], "r");
 *
 * // Call csv_context_file() to initialise the CsvContext object
 * // to read read CSV data from an open file.
 * csv_context_file(&csv, f);
 *
 * // csv_read_record() reads a row from the file.
 * // It will return 0 when it reaches the end of the file
 * while(csv_read_record(&csv)) {
 *    int j;
 *    // You can use csv_count() to retrieve the number of fields
 *    // read from the file.
 *    // csv_field() can then be used to access an individual field.
 *    for(j = 0; j < csv_count(&csv); j++) {
 *      printf("[%s]", csv_field(&csv,j));
 *    }
 *    printf("\n");
 * }
 * fclose(f);
 * ```
 *
 * ## License
 *
 * Author: Werner Stoop
 *
 * This is free and unencumbered software released into the public domain.
 *
 * See <http://unlicense.org/> for details
 *
 * [wiki]: https://en.wikipedia.org/wiki/Comma-separated_values
 * [RFC]: https://datatracker.ietf.org/doc/html/rfc4180
 * [Repici]: https://www.creativyst.com/Doc/Articles/CSV/CSV01.shtml
 */


/**
 * ## Configuration
 *
 * These macros can be defined before including **csvstrm.h** in your
 * C file to control the behaviour of the library.
 *
 * * `CSV_DELIMITER` -
 *   The delimiter to separate fields (columns) in each record (row)
 *   It defaults to `','`.
 * * `CSV_BUFFER_SIZE` -
 *   While each record is being read, the characters from the file
 *   are copied to an internal buffer. This controls the size of
 *   that internal buffer.
 * * `CSV_READ_BUFFER_SIZE` -
 *   This controls the size of the second internal buffer that
 *   stores raw bytes as they are read from the input before they're
 *   processed.
 * * `CSV_MAX_FIELDS` -
 *   The maximum number of fields expected per record.
 * * `CSV_TRIM` -
 *   Determines whether leading and trailing whitespace characters will
 *   be trimmed from fields by the parser.  \
 *   For example, consider a CSV section `..., foo ,...`:  \
 *   If `CSV_TRIM` is non-zero the field will be returned as `"foo"`.  \
 *   If it is 0 then the whitespace will be left intact, so it will be
 *   returned as `" foo "`
 *
 * These macros _must_ be the same in all files that include **csvstrm.h**.
 */

#  ifndef CSV_DELIMITER
#    define CSV_DELIMITER   ','
#  endif

#  ifndef CSV_BUFFER_SIZE
#    define CSV_BUFFER_SIZE 256
#  endif

#  ifndef CSV_READ_BUFFER_SIZE
#    define CSV_READ_BUFFER_SIZE 64
#  endif

#  ifndef CSV_MAX_FIELDS
#    define CSV_MAX_FIELDS 32
#  endif

#  ifndef CSV_TRIM
#    define CSV_TRIM 1
#  endif

/**
 * ## Definitions
 *
 * ### `csv_read_data_fun`
 *
 * `typedef int (*csv_read_data_fun)(char *b, int n, void *d);`
 *
 * Prototype for functions that can read CSV data.
 *
 * `b` is a pointer to a buffer that will be filled with chars from
 * the input files. `n` contains the size in bytes of the buffer.
 * `d` is a pointer to some structure where the data is read from.
 *
 * For example, when reading a CSV file from a ZIP archive, `d` might
 * point to the structure that the ZIP library to encapsulate the archive.
 *
 * The function should return 0 if it reaches the end of the input data,
 * non-zero otherwise.
 *
 * See `csv_context_custom()` in section [initialising the csvcontext](#initialising-the-csvcontext)
 */

typedef int (*csv_read_data_fun)(char *b, int n, void *d);

/**
 * ### `enum csv_error_code`
 *
 * * `CSV_OK` -
 *   No error
 * * `CSV_ERR_BUFFER` -
 *   The buffer used to store field data internally is full.
 *   It is too small for the record (row) you're reading.  \
 *   Increase `CSV_BUFFER_SIZE`.
 * * `CSV_ERR_FIELDS` -
 *   There are too many fields (columns) in the record.  \
 *   Increase `CSV_MAX_FIELDS`.
 * * `CSV_ERR_BAD_QUOTE` -
 *   A quoted field is incorrectly formatted.
 * * `CSV_ERR_LINE_END` -
 *   There is a problem with a line ending.
 */
enum csv_error_code {
    CSV_OK = 0,
    CSV_ERR_BUFFER, /* increase CSV_BUFFER_SIZE */
    CSV_ERR_FIELDS, /* increase CSV_MAX_FIELDS */
    CSV_ERR_BAD_QUOTE,
    CSV_ERR_LINE_END,
};

/**
 * ### `typedef struct CsvContext CsvContext;`
 *
 * Structure that contains the state of the CSV stream parser.
 *
 * The fields in the structure should not be manipulated directly,
 * but these are some members of interest:
 *
 * * `char *fields[CSV_MAX_FIELDS]` -
 *   The array of pointers that contain the fields after parsing a record.
 *   Rather use `csv_field()` to access the individual fields.
 * * `int nf` -
 *   The number of fields parsed from a record.
 *   Rather use `csv_count()` to read this value.
 * * `enum csv_error_code err` -
 *   An error code that may have resulted from parsing the record.
 *   Rather use `csv_get_error()` to retrieve this value.
 *
 * Section [initialising the csvcontext](#initialising-the-csvcontext)
 * below describes how to initialise the structure to read CSV data.
 */
typedef struct CsvContext {

    /* Determines where the data is read from */
    csv_read_data_fun get_data;
    void *data;

    /* The internal buffer, where bytes are read into
    from the file, but before they're processed. */
    char raw_buffer[CSV_READ_BUFFER_SIZE];
    int in_pos;
    int last_char;

    /* Where the data for the fields are stored.
    The values in `fields` are a pointers into this buffer */
    char buffer[CSV_BUFFER_SIZE];

    /* The fields that have been parsed from the file */
    char *fields[CSV_MAX_FIELDS];
    int nf;

    /* Error code? */
    enum csv_error_code err;

} CsvContext;


#ifdef EOF /* EOF will be defined if <stdio.h> is #included */

/**
 * ## Initialising the `CsvContext`
 *
 * ### `void csv_context_file(CsvContext *csv, FILE *file)`
 *
 * Initialises a `CsvContext` structure to read data from a file
 * pointed to by `file`.
 */
void csv_context_file(CsvContext *csv, FILE *file);

/**
 * ### `void csv_context_file_limit(CsvContext *csv, struct csv_read_limit *ll)`
 *
 * Initialises a `CsvContext` structure to read data from a file, but it will
 * only read a limited number of bytes from the file.
 *
 * (The intended use-case is where a CSV file has been concatenated with other
 * files into an archive file)
 *
 * The `csv_read_limit` structure `ll` is defined as follows:
 *
 * ```
 * struct csv_read_limit {
 *   FILE *f;
 *   int limit;
 * };
 * ```
 *
 * where `f` is the file to read from and `limit` is the maximum
 * number of bytes that will be read from the file.
 */
struct csv_read_limit {
    FILE *f;
    int limit;
};

void csv_context_file_limit(CsvContext *csv, struct csv_read_limit *ll);
#endif

/**
 * ### `void csv_context_custom(CsvContext *csv, csv_read_data_fun fun, void *data)`
 *
 * Initialises a `CsvContext` with a custom function `fun` that will read bytes
 * from an object `data`.
 */
void csv_context_custom(CsvContext *csv, csv_read_data_fun fun, void *data);

/**
 * ## Reading records
 *
 * ### `int csv_read_record(CsvContext *csv)`
 *
 * Reads a record from the CSV file.
 *
 * It returns the number of fields that were read from the record.
 * If the number of fields does not match the number of fields expected
 * then `csv_get_error()` can be used to retrieve the error code.
 *
 * ### `int csv_count(CsvContext *csv)`
 *
 * Get the number of fields in the last record that was read by
 * `csv_read_record()`.
 *
 * ### `const char *csv_field(CsvContext *csv, int i)`
 *
 * Get the `i`'th field of the last record that was read by
 * `csv_read_record()`.
 *
 * ### `enum csv_error_code csv_get_error(CsvContext *csv)`
 *
 * Retrieves an error code (if any) from the `CsvContext`.
 * The error codes are described in Subsection [enum csv_error_code](#enum-csv_error_code).
 */
int csv_read_record(CsvContext *csv);

int csv_count(CsvContext *csv);

const char *csv_field(CsvContext *csv, int i);

enum csv_error_code csv_get_error(CsvContext *csv);

/* *********************************************************************** */

#  ifdef CSV_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef __cplusplus
#  define CAST(x, y)   (x)y
#else
#  define CAST(x, y)   y
#endif

static int _csv_get_char(CsvContext *csv) {
    char c = 0;
    if(csv->last_char == EOF) {
        return EOF;
    } else if(csv->last_char) {
        c = csv->last_char;
        csv->last_char = 0;
        return c;
    }
    if(csv->in_pos >= CSV_READ_BUFFER_SIZE || (c = csv->raw_buffer[csv->in_pos++]) == '\0') {
        int cnt = csv->get_data(csv->raw_buffer, CSV_READ_BUFFER_SIZE - 1, csv->data);
        if(!cnt) {
            csv->last_char = EOF;
            return EOF;
        }
        csv->in_pos = 0;
        c = csv->raw_buffer[csv->in_pos++];
    }
    return c;
}

static void _csv_unget_char(CsvContext *csv, int c) {
    csv->last_char = c;
}

int csv_read_record(CsvContext *csv) {
    int c = 0;
    size_t start, bump = 0;

    enum parse_state {
        RECORD_START, FIELD_START, FIELD, QUOTE, FIELD_END, RECORD_END
    } state = RECORD_START;

    if(csv->last_char == EOF) return 0;

    csv->nf = 0;
    csv->err = CSV_OK;

    for(;;) {
        switch(state) {
            case RECORD_START:
                c = _csv_get_char(csv);
                if(c == EOF)
                    return 0;
                state = FIELD_START;
                _csv_unget_char(csv, c);
            break;
            case FIELD_START:
                if(csv->nf == CSV_MAX_FIELDS) {
                    csv->err = CSV_ERR_FIELDS;
                    return csv->nf;
                }
                c = _csv_get_char(csv);
#if CSV_TRIM
                while(strchr(" \t\v\f", c))
                    c = _csv_get_char(csv);
#endif
                csv->fields[csv->nf] = &csv->buffer[bump];
                if(c == '\"')
                    state = QUOTE;
                else {
                    _csv_unget_char(csv, c);
                    start = bump;
                    state = FIELD;
                }
            break;
            case FIELD:
                c = _csv_get_char(csv);
                if(c == '\r') {
                    c = _csv_get_char(csv);
                    if(c != '\n') {
                        csv->err = CSV_ERR_LINE_END;
                        return csv->nf;
                    }
                }

                if(c == EOF || c == '\n' || c == CSV_DELIMITER) {
#if CSV_TRIM
                    while(bump > start && strchr(" \t\v\f", csv->buffer[bump-1]))
                        bump--;
#endif
                    state = c == CSV_DELIMITER ? FIELD_END : RECORD_END;
                } else {
                    if(bump == CSV_BUFFER_SIZE - 1) {
                        csv->err = CSV_ERR_BUFFER;
                        return csv->nf;
                    }
                    csv->buffer[bump++] = c;
                }
            break;
            case QUOTE:
                c = _csv_get_char(csv);
                if(c == EOF) {
                    csv->err = CSV_ERR_BAD_QUOTE;
                    return csv->nf;
                }
                if(c == '\"') {
                    c = _csv_get_char(csv);
                    if(c != '\"') {
#if CSV_TRIM
                        while(strchr(" \t\v\f", c))
                            c = _csv_get_char(csv);
#endif
                        if(c == EOF || c == '\n') {
                            state = RECORD_END;
                        } else if(c == CSV_DELIMITER) {
                            state = FIELD_END;
                        } else {
                            csv->err = CSV_ERR_BAD_QUOTE;
                            return csv->nf;
                        }
                        break;
                    }
                }
                if(bump == CSV_BUFFER_SIZE - 1) {
                    csv->err = CSV_ERR_BUFFER;
                    return csv->nf;
                }
                csv->buffer[bump++] = c;
            break;
            case FIELD_END:
            case RECORD_END:
                if(bump == CSV_BUFFER_SIZE - 1) {
                    csv->err = CSV_ERR_BUFFER;
                    return csv->nf;
                }
                csv->buffer[bump++] = '\0';
                csv->nf++;
                if(state == RECORD_END)
                    return csv->nf;
                else
                    state = FIELD_START;
            break;
        }
    }
    /*return 0;*/
}

void csv_context_custom(CsvContext *csv, csv_read_data_fun fun, void *data) {
    csv->get_data = fun;
    csv->data = data;
    csv->last_char = 0;
    csv->in_pos = CSV_READ_BUFFER_SIZE;
    csv->nf = 0;
    csv->err = CSV_OK;
}

static int _csv_file_input_get_line(char *str, int num, void *data) {
    size_t read;
    FILE *file = CAST(FILE*, data);
    if(feof(file))
        return 0;
    read = fread(str, 1, num, file);
    str[read] = '\0';
    if(!read)
        return 0;
    return 1;
}

void csv_context_file(CsvContext *csv, FILE *file) {
    assert(file != NULL);
    csv_context_custom(csv, _csv_file_input_get_line, file);
}

static int _csv_file_input_get_line_limit(char *str, int num, void *data) {
    size_t read;
    struct csv_read_limit *ll = CAST(struct csv_read_limit *, data);
    if(!ll->limit) return 0;
    num--;
    if(num > ll->limit)
        num = ll->limit;
    read = fread(str, 1, num, ll->f);
    str[read] = '\0';
    if(!read)
        return 0;
    ll->limit -= strlen(str);
    return 1;
}

void csv_context_file_limit(CsvContext *csv, struct csv_read_limit *ll) {
    assert(ll->f != NULL);
    assert(ll->limit > 0);
    csv_context_custom(csv, _csv_file_input_get_line_limit, ll);
}

int csv_count(CsvContext *csv) {
    return csv->nf;
}

const char *csv_field(CsvContext *csv, int i) {
    if(i < 0 || i >= csv->nf) return "";
    return csv->fields[i];
}

enum csv_error_code csv_get_error(CsvContext *csv) {
    return csv->err;
}

#  endif /* CSV_IMPLEMENTATION */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CSV_STREAM_H */
