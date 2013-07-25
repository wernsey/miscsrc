/*
 * A set of functions for the manipulation of RFC 4180 style
 * Comma Separated Values (CSV) files.
 *
 * See csv.h for more info
 *
 * This is free and unencumbered software released into the public domain.
 * http://unlicense.org/
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

#include "csv.h"
#include "utils.h"

#ifdef _MSC_VER
/* For Visual C++ 6.0
 * (I don't know about newer versions yet)
 */
#	define vsnprintf _vsnprintf
#endif

/* The size of the internal buffer used by csv_setx() */
#define CSV_SETX_BUFFER_SIZE 512

/*
 * Trim whitespace at thestart and end of fields?
 * This does not apply to quoted fields
 */
#define TRIM_SPACES 0

/* Various error codes */
#define ER_OK		 		 1
#define ER_MEM_FAIL  		-1
#define ER_IOW_FAIL  		-2
#define ER_IOR_FAIL  		-3
#define ER_INV_PARAM 		-4
#define ER_EXPECTED_EOS	 	-5
#define ER_BAD_QUOTEEND		-6

static int csv_set_int(csv_file *csv, int row, int col, char *value);

const char *csv_errstr(int err)
{
	switch(err)
	{
		case ER_OK: return "Success";
		case ER_MEM_FAIL: return "Out of memory";
		case ER_IOW_FAIL: return "Unable to open file for writing";
		case ER_IOR_FAIL: return "Unable to read file";
		case ER_INV_PARAM: return "Invalid parameter";
		case ER_EXPECTED_EOS : return "Unterminated string";
		case ER_BAD_QUOTEEND : return "Expected a field or record separator after the \"";
	}
	return "Unknown";
}

/* Allocates memory for a CSV file structure */
csv_file *csv_create(int def_rows, int def_cols)
{
	csv_file *csv;
	int i, j;
	
	if(def_rows <= 0)
		def_rows = CSV_DEFAULT_ROWS;
	if(def_cols <= 0)
		def_cols = CSV_DEFAULT_COLS;		
	
	csv = malloc(sizeof *csv);
	if(!csv) return NULL;	
	
	csv->rows = calloc(def_rows, sizeof *csv->rows);
	if(!csv->rows)
	{
		free(csv);
		return NULL;
	}	
	csv->arows = def_rows;
	csv->nrows = 0;
	
	csv->def_cols = def_cols;
	
	for(i = 0; i < csv->arows; i++)
	{
		csv->rows[i].cols = calloc(def_cols, sizeof *csv->rows[i].cols);
		if(!csv->rows[i].cols)
		{
			for(j = 0; j < i; j++) free(csv->rows[j].cols);
			free(csv->rows);
			free(csv);
			return NULL;
		}
		
		csv->rows[i].acols = def_cols;
		csv->rows[i].ncols = 0;
	}
	
	return csv;
}

/* Deallocs memory allocated by csv_create() */
void csv_free(csv_file *csv)
{
	int r,c;
	for(r = 0; r < csv->arows; r++)
	{
		for(c = 0; c < csv->rows[r].acols; c++)
			if(csv->rows[r].cols[c])
				free(csv->rows[r].cols[c]);
		free(csv->rows[r].cols);
	}
	free(csv->rows);
	free(csv);
}

#define RETERR(code) do{if(err)*err = code;return NULL;}while(0)
#define ERREND(code) do{if(err)*err = code;goto error;}while(0)

/* loads a CSV file from disk */
csv_file *csv_load(const char *filename, int *err, int *line)
{
	csv_file *csv;
	char *buffer, *p;
	int r = 0, c = 0;
	
	if(err) *err = ER_OK;
	if(line) *line = 1;
	
	if(!filename)
		RETERR(ER_INV_PARAM);
				
	csv = csv_create(0,0);
	if(!csv) 
		RETERR(ER_MEM_FAIL);
	
	/* My approach is to read the entire file into memory and then 
	 * parse it from there. 
	 * I Acknowledge that this is not the most efficient way of doing it,
	 * but it does simplify the parsing somewhat.
	 */
	buffer = my_readfile(filename);
	if(!buffer)
		ERREND(ER_IOR_FAIL);
	
	for(p = buffer; *p;)
	{
		if(strchr(" \t",p[0]))
		{
			/* This is to prevent space between the start of a field 
			and a " from confusing the parser */
			char *q;
			for(q = p + 1; strchr(" \t",q[0]); q++);
			if(q[0] == '\"')
				p = q;
		}
		
		if(!strncmp(p,"\r\n", 2)) 
		{
			/* official line endings */
			r++;
			c = 0;
			p+=2;
			if(line) (*line)++;
		}
		else if(strchr("\r\n",p[0]))
		{
			/* alternative line endings */
			r++;
			c = 0;	
			p++;
			if(line) (*line)++;
		}
		else if(*p == ',')
		{
			/* A new or empty field */
			c++;
			p++;
		}
		else if(*p == '\"')
		{
			/* A quoted field */
			char *q = p, *s, *t;			
			do {
				q++;
				if(*q == '\0')
					ERREND(ER_EXPECTED_EOS);
				else if(q[0] == '\"' && q[1] == '\"')
					q+=2;
			} while(*q != '\"');
			
			s = malloc(q - p);
			if(!s)
				ERREND(ER_MEM_FAIL);
			t = s;
			p++;			
			while(p < q)
			{
				if(*p == '\"' && *(p+1) == '\"')
				{
					*(t++) = *(p++);
					p++;					
				}
				else
					*(t++) = *(p++);
			}		
			
			*t = '\0';				
			
			/* Skip any whitespace after the closing quote */
			for(p++; p[0] && !strchr(",\r\n",p[0]);p++)
				if(!strchr(" \t",p[0]))
					ERREND(ER_BAD_QUOTEEND);
			
			csv_set_int(csv, r, c, s);
		}
		else
		{
			/* A normal field */
			char *q, *s, *t;
			
#if TRIM_SPACES			
			/* Trim leading whitespace */
			while(p[0] && strchr(" \t",p[0])) p++;
#endif
			for(q = p;q[0] && !strchr(",\r\n",q[0]); q++);				
			
			s = malloc(q - p + 1);
			if(!s)
				ERREND(ER_MEM_FAIL);
			t = s;
			while(p < q)
			{
				*(t++) = *(p++);
			}		
			*t = '\0';
			
#if TRIM_SPACES
			/* Trim trailing whitespace */
			while(t > s && strchr(" \t",t[-1]))
				*(--t) = '\0';
#endif		
				
			csv_set_int(csv, r, c, s);
		}
	}
		
	free(buffer);
	return csv;
	
error:
	csv_free(csv);
	return NULL;
}

/* Saves a CSV file to disk */
int csv_save(csv_file *csv, const char *filename)
{
	int r, c;	
	FILE *f = stdout;
	
	if(!csv || !filename) return ER_INV_PARAM;
	
	f = fopen(filename, "w");
	if(!f) return ER_IOW_FAIL;
		
	for(r = 0; r < csv->nrows; r++)
	{
		for(c = 0; c < csv->rows[r].ncols; c++)
		{
			const char *cell = csv->rows[r].cols[c];
			if(cell)
			{
				if(strpbrk(cell, ",\"\r\n"))
				{
					const char *p;
					fputc('\"',f);
					for(p = cell; *p; p++)
					{
						if(*p == '\"')
							fputc('\"', f);
						fputc(*p, f);
					}
					fputc('\"',f);
				}
				else
					fputs(cell, f);
			}
			
			if(c < csv->rows[r].ncols - 1)
				fputc(',',f);
		}
		fputs(CSV_LINE_TERMINATOR,f);
	}
	
	return 1;
}

/* Returns the number of rows in a CSV file */
int csv_rowcount(csv_file *csv)
{
	if(!csv) return 0;
	return csv->nrows;
}

/* Returns the number of columns in a CSV file */
int csv_colcount(csv_file *csv, int row)
{
	if(!csv || row >= csv->nrows) return 0;	
	return csv->rows[row].ncols;
}

/* Retrieves the value of a cell */
const char *csv_get(csv_file *csv, int row, int col)
{
	if(!csv || row >= csv->nrows || col >= csv->rows[row].ncols || row < 0 || col < 0 || !csv->rows[row].cols[col])
		return "";
	return csv->rows[row].cols[col];
}

/* Sets the value at cell [row,col] in the csv_file (internal function).
 * This function is used internally because it assumes this module is already
 * the owner of 'value' - you'll notice that csv_set() just calls this function
 * with a strdup()ed value.
 */
static int csv_set_int(csv_file *csv, int row, int col, char *value)
{
	csv_row *rp;
	
	if(!csv || row < 0 || col < 0) return ER_INV_PARAM;
	
	if(row >= csv->nrows)
	{
		if(row >= csv->arows)
		{
			/* Need to allocate more rows */
			int i, ns = MY_MAX(row+1,csv->arows + (csv->arows >> 1));	
			void *op = csv->rows;
			
			if(ns <= 2) ns = 3;
			
			csv->rows = realloc(csv->rows, ns * sizeof *csv->rows);
			if(!csv->rows)
			{
				csv->rows = op;
				return ER_MEM_FAIL;
			}
			
			/* Initialise the new rows */
			for(i = csv->arows; i < ns; i++)
			{
				csv->rows[i].cols = calloc(csv->def_cols, sizeof *csv->rows[i].cols);
				if(!csv->rows[i].cols)
					return ER_MEM_FAIL;
				csv->rows[i].acols = csv->def_cols;
				csv->rows[i].ncols = 0;
			}
			
			csv->arows = ns;
		}
		
		csv->nrows = row + 1;
		assert(csv->nrows <= csv->arows);		
	}	
	
	assert(row < csv->nrows);
	rp = &csv->rows[row];
	
	if(col >= rp->ncols)
	{
		if(col >= rp->acols)
		{
			/* Need to reallocate the columns */
			int i, ns = MY_MAX(col+1, rp->acols + (rp->acols >> 1));
			void *op = rp->cols;
			
			if(ns <= 2) ns = 3;
			
			rp->cols = realloc(rp->cols, ns * sizeof *rp->cols);
			if(!rp->cols)
			{
				rp->cols = op;
				return ER_MEM_FAIL;
			}

			for(i = rp->acols; i < ns; i++)
				rp->cols[i] = NULL;

			rp->acols = ns;
		}	

		rp->ncols = col + 1;
		assert(rp->ncols <= rp->acols);
	}
		
	assert(col < rp->ncols);
	
	if(rp->cols[col])
		free(rp->cols[col]);
	rp->cols[col] = value;
	
	if(rp->cols[col]) 
		return ER_OK;
	
	return ER_MEM_FAIL;
}

/* Sets the value at cell [row,col] in the csv_file */
int csv_set(csv_file *csv, int row, int col, const char *value)
{
	return csv_set_int(csv, row, col, my_strdup(value));
}

/* Sets a specific cell in the CSV file to a value with a printf()-style format */
int csv_setx(csv_file *csv, int row, int col, const char *fmt, ...)
{
	char buffer[CSV_SETX_BUFFER_SIZE];
	va_list arg;
	va_start (arg, fmt);
	vsnprintf (buffer, CSV_SETX_BUFFER_SIZE, fmt, arg);
	va_end (arg);
	
	return csv_set(csv, row, col, buffer);
}
