/*1 csv.h
 *# A set of functions for the manipulation of RFC 4180 style
 *# Comma Separated Values (CSV) files.\n
 *#
 *# The API centers around the structure {{~~csv_file}} which represents a CSV
 *# file in memory. The API has these functions:
 *{
 ** The {{~~csv_load()}} function is used to load a CSV file from disc into a new
 *#   {{csv_file}} structure. 
 ** Alternatively an empty {{csv_file}} structure can be created through the 
 *#   {{~~csv_create()}} function.
 ** The {{~~csv_get()}}, {{~~csv_set()}} and {{~~csv_setx()}} functions are used to access 
 *#   individual fields within the {{csv_file}}.
 ** {{~~csv_rowcount()}} and {{~~csv_colcount()}} are used to get the size of the {{csv_file}}.
 ** {{~~csv_save()}} is used to write the {{csv_file}} to disc.
 ** {{~~csv_free()}} deallocates the memory allocated to a {{csv_file}} when it is no longer in use.
 ** {{~~csv_errstr()}} provides text descriptions of error codes.
 *}
 *N Rows and columns in the CSV file structure are indexed from 0
 *# (The first row is row 0, not row 1, and the last row is row N-1).
 *#
 *# With regards to RFC4180 [1], note the following:
 *{	  
 ** As required in section 2.1, when saving a CSV file, records are terminated 
 *#  by a CRLF ({{"\r\n"}}) sequence. This can be changed by redefining 
 *#  {{CSV_LINE_TERMINATOR}} in the header file if desired. 
 **	The existence of the header line mentioned in section 2.3 is entirely up to 
 *#  the application. This module does not care about it.
 ** Section 2.4 states that each record should have the same number of fields.
 *#  This module does not enforce such a restriction.
 ** When writing CSV files, the last record will have a line break (section 2.2)
 ** Section 2.4 of the RFC also specifies that spaces are considered part of the field.
 *#  However, other sources [2] specifiy that leading and trailing spaces should 
 *#  be trimmed unless they are enclosed in quotes. The default behaviour of 
 *#  this module is to not trim leading and trailing whitespace in order to conform
 *#  to the RFC, but this behaviour can be changed by defining {*TRIM_SPACES*} 
 *#  at the top of {*csv.c*} to a non-zero value.
 *}
 *2 References:
 *{
 ** [1] RFC 4180 available at http://tools.ietf.org/html/rfc4180
 ** [2] Repici, J., "HOW-TO: The Comma Separated Value (CSV) File Format", 2004,
 *#  http://www.creativyst.com/Doc/Articles/CSV/CSV01.htm
 *} 
 *2 License
 *[
 *# Author: Werner Stoop
 *# This software is provided under the terms of the unlicense.
 *# See http://unlicense.org/ for more details.
 *]
 *2 API
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*@ struct csv_row
 *# Structure representing a single row in the CSV file
 */
typedef struct csv_row
{
	int ncols; /* number of columns */
	int acols; /* allocated columns; must be >= ncols*/
	char **cols;
} csv_row;

/*@ struct ##csv_file
 *# Structure representing a CSV file in memory.
 */
typedef struct csv_file
{
	int nrows; /* number of rows */
	int arows; /* allocated rows; must be >= nrows*/
	csv_row *rows;
	
	int def_cols;
	
} csv_file;

/*@ CSV_LINE_TERMINATOR
 *# The record terminator.
 */
#define CSV_LINE_TERMINATOR "\r\n"

/*@ CSV_DEFAULT_ROWS
 *# Default number of rows in a CSV structure
 */
#define CSV_DEFAULT_ROWS 10

/*@ CSV_DEFAULT_COLS
 *# Default number of columns in a CSV structure
 */
#define CSV_DEFAULT_COLS 10

/*@ csv_file *##csv_create(int def_rows, int def_cols)
 *# Allocates memory for a {{csv_file}} structure and initialises it.\n
 *# {{def_rows}} and {{def_cols}} specifies the number of rows and columns 
 *# expected in the structure (although the structure will be resized if it
 *# grows beyond these values).\n
 *# If {{def_rows}} or {{def_cols}} is set to 0, {{CSV_DEFAULT_ROWS}} and
 *# {{CSV_DEFAULT_COLS}} will be used instead, respectively.
 */
csv_file *csv_create(int def_rows, int def_cols);

/*@ csv_file *##csv_load(const char *filename, int *err, int *line)
 *# Loads a CSV file.
 *# It returns {{NULL}} on failure, in which case {{err}} will contain the error code
 *# (see {{~~csv_errstr()}}) and {{line}} will contain ({{err}} and {{line}} may be {{NULL}}).
 */
csv_file *csv_load(const char *filename, int *err, int *line);

/*@ void ##csv_free(csv_file *csv)
 *# Deallocates memory previously allocated to a {{csv_file}} though {{~~csv_create()}}
 *# or {{~~csv_load()}}
 */
void csv_free(csv_file *csv);

/*@ int ##csv_save(csv_file *csv, const char *filename)
 *# Saves the {{csv_file}} file to a file named in {{filename}} on the disk.
 *# It returns 1 on success and an error code on failure (see {{~~csv_errstr()}}).
 */
int csv_save(csv_file *csv, const char *filename);

/*@ int ##csv_rowcount(csv_file *csv)
 *# Retruns the number of rows in the {{csv_file}}.
 */
int csv_rowcount(csv_file *csv);

/*@ int ##csv_colcount(csv_file *csv, int row)
 *# Returns the number of columns in row {{row}} of {{csv}}
 */
int csv_colcount(csv_file *csv, int row);

/*@ const char *##csv_get(csv_file *csv, int row, int col)
 *# Retrieves the value at cell [{{row}},{{col}}] of {{csv}}.\n
 *# If the cell at [{{row}},{{col}}] does not exist or {{row}} or {{col}}
 *# are invalid, it will return {{""}} instead of failing.
 */
const char *csv_get(csv_file *csv, int row, int col);

/*@ int ##csv_set(csv_file *csv, int row, int col, const char *value)
 *# Sets the cell at [{{row}},{{col}}] of {{csv}} to the string {{value}}.\n
 *# It returns 1 on success, or an error code on failure (see {{~~csv_errstr()}}).
 */
int csv_set(csv_file *csv, int row, int col, const char *value);

/*@ int ##csv_setx(csv_file *csv, int row, int col, const char *fmt, ...)
 *# Sets the cell at [{{row}},{{col}}] of {{csv}} to a formatted string.\n
 *# The parameter {{fmt}} works like {{printf()}}, and uses the same escape sequences
 *# (it uses {{vsnprintf()}} internally).
 */
int csv_setx(csv_file *csv, int row, int col, const char *fmt, ...);

/*@ const char *##csv_errstr(int err)
 *# Returns a textual description of the error code {{err}}
 */
const char *csv_errstr(int err);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
