/**
 * # Stream Tokenizer
 *
 * Single header stream tokenizer library.
 *
 * It is used for [Lexical analysis][lexer] (or _lexing_ or _tokenisation_)
 * when creating parsers for text.
 *
 * Inspired by (but not entirely compatible with) the [java.io.StreamTokenizer][java]
 * class in the Java standard library.
 *
 * ## License
 *
 * Author: Werner Stoop
 *
 * This is free and unencumbered software released into the public domain.
 *
 * See <http://unlicense.org/> for details
 *
 * [lexer]: https://en.wikipedia.org/wiki/Lexical_analysis
 * [java]: https://docs.oracle.com/javase/7/docs/api/java/io/StreamTokenizer.html
 *
 */
#ifndef STRMTOK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ## Configuration
 *
 * These macros can be defined before including **csvstrm.h** in your
 * C file to control the behaviour of the library.
 *
 * * `ST_STATIC` - define this to have all the functions in the
 *   library declared `static`. This is useful if you need to
 *   use the libary to parse different
 * * `ST_BUFFER_SIZE` (default: 256) -
 *   Size of the buffer used to store tokens.
 * * `ST_READ_BUFFER_SIZE` (default: 64) -
 *   This controls the size of the second internal buffer that
 *   stores raw bytes as they are read from the input before they're
 *   processed.
 * * `ST_DEFAULT_LOWERCASE_MODE` (default: 0) -
 *   default value for `StrmTok.lowercaseMode`
 * * `ST_DEFAULT_SIGNIFICANT_EOL` (default: 0) -
 *   default value for `StrmTok.significantEol`
 * * `ST_DEFAULT_COMMENT_CHARS` (default: `"#"`) -
 *   default value for `StrmTok.comment_chars`
 * * `ST_DEFAULT_SINGLE_COMMENT` (default: `"//"`) -
 *   default value for `StrmTok.single_comment`
 * * `ST_DEFAULT_MULTI_COMMENT_START` (default: `"/\*"`) -
 *   default value for `StrmTok.multi_comment_start`
 * * `ST_DEFAULT_MULTI_COMMENT_END` (default: `"*\/"`) -
 *   default value for `StrmTok.multi_comment_end`
 * * `ST_DEFAULT_QUOTE_CHARS`  (default: `"\"'"`) -
 *   default value for `StrmTok.quote_chars`
 * * `ST_DEFAULT_MULTI_STRING_START` (default: `"\"\"\""`) -
 *   default value for `StrmTok.multi_string_start`
 * * `ST_DEFAULT_MULTI_STRING_END` (default: `"\"\"\""`) -
 *   default value for `StrmTok.multi_string_end`
 * * `ST_DEFAULT_WORD_CHARS` (default: `"_"`) -
 *   default value for `StrmTok.word_chars`
 * * `ST_DEFAULT_OPERATORS` (default: `NULL`) -
 *   default value for `StrmTok.operators`
 *
 * **Note**: Unless you declare `ST_STATIC`,
 * the buffer sizes _must_ be the same in all files that include **strmtok.h**.
 */
#ifdef ST_STATIC
#  define _ST_EXPORT static
#else
#  define _ST_EXPORT
#endif

#  ifndef ST_BUFFER_SIZE
#    define ST_BUFFER_SIZE 256
#  endif

#  ifndef ST_READ_BUFFER_SIZE
#    define ST_READ_BUFFER_SIZE 64
#  endif

#  ifndef ST_DEFAULT_LOWERCASE_MODE
#    define ST_DEFAULT_LOWERCASE_MODE 0
#  endif

#  ifndef ST_DEFAULT_SIGNIFICANT_EOL
#    define ST_DEFAULT_SIGNIFICANT_EOL 0
#  endif

#  ifndef ST_DEFAULT_COMMENT_CHARS
#    define ST_DEFAULT_COMMENT_CHARS "#"
#  endif

#  ifndef ST_DEFAULT_SINGLE_COMMENT
#    define ST_DEFAULT_SINGLE_COMMENT "//"
#  endif

#  ifndef ST_DEFAULT_MULTI_COMMENT_START
#    define ST_DEFAULT_MULTI_COMMENT_START "/*"
#  endif

#  ifndef ST_DEFAULT_MULTI_COMMENT_END
#    define ST_DEFAULT_MULTI_COMMENT_END "*/"
#  endif

#  ifndef ST_DEFAULT_QUOTE_CHARS
#    define ST_DEFAULT_QUOTE_CHARS "\"'"
#  endif

#  ifndef ST_DEFAULT_MULTI_STRING_START
#    define ST_DEFAULT_MULTI_STRING_START "\"\"\""
#  endif

#  ifndef ST_DEFAULT_MULTI_STRING_END
#    define ST_DEFAULT_MULTI_STRING_END "\"\"\""
#  endif

#  ifndef ST_DEFAULT_WORD_CHARS
#    define ST_DEFAULT_WORD_CHARS "_"
#  endif

#  ifndef ST_DEFAULT_OPERATORS
#    define ST_DEFAULT_OPERATORS NULL
#  endif

#  ifndef ST_UNGET_COUNT
#    define ST_UNGET_COUNT 8
#  endif

#if defined(STRMTOK_TEST)
#  include <stdio.h>
#endif

/**
 * ## Definitions
 *
 * ### `st_read_data_fun`
 *
 * `typedef int (*st_read_data_fun)(char *b, int n, void *d)`
 *
 * Prototype for functions that can read input data.
 *
 * `b` is a pointer to a buffer that will be filled with chars from
 * the input files. `n` contains the size in bytes of the buffer.
 * `d` is a pointer to some structure where the data is read from.
 *
 * The function should return 0 if it reaches the end of the input data,
 * non-zero otherwise.
 *
 */

typedef int (*st_read_data_fun)(char *b, int n, void *d);

/**
 * ### `enum st_token`
 *
 * Types of tokens that may be read from the input stream.
 * `st_next_token()` will return one of these, or an ASCII character
 * if the input matches one of the operators.
 *
 *  * `ST_EOF` - End of the file (or input stream) was reached.
 *  * `ST_ERROR` - An error occurred. Use `StrmTok.error_desc` to get
 *      a description of the error, and `StrmTok.lineno` to get the line
 *      number in the input stream where the error occurred
 *  * `ST_EOL` - End of line.
 *      This will only be returned if `StrmTok.significantEol` is true.
 *  * `ST_WORD` - The token was a _word_: alphanumeric charecters or
 *      characters in `StrmTok.word_chars`.
 *  * `ST_STRING` - The token was a quote-delimited string.
 *  * `ST_NUMBER` - The token was a numeric value.
 */
enum st_token {
    ST_ERROR = -1,
    ST_EOF = 0,
    ST_EOL,
    ST_WORD,
    ST_STRING,
    ST_NUMBER,
};

/**
 * ### `typedef struct StrmTok StrmTok`
 *
 * Structure that tracks the tokenization.
 *
 * These members are used to configure the parser:
 *
 * * `int lowercaseMode` - If non-zero, when a `ST_WORD` token
 *   is read, it is converted to all lowercase characters.
 *   This may be useful for parsers where keywords are not case sensitive.
 * * `int significantEol` - If non-zero, the parser will treat carriage
 *   return characters as tokens and return `ST_EOL`. If it is zero, the
 *   carriage return characters are ignored like other whitespace.
 * * `const char *comment_chars` - Characters that indicate the start of a
 *   single line comment, if comments start with a single character.
 * * `const char *single_comment` - a string that indicates the start of a
 *   single line comment, if comments start with a multiple characters.
 * * `const char *multi_comment_start`- a string that indicates the start of a
 *   multi line comment.
 * * `const char *multi_comment_end`- a string that indicates the end of a
 *   multi line comment.
 * * `const char *quote_chars` - Characters that indicate the start/end of
 *   quoted strings (for which `st_next_token()` returns `ST_STRING`).
 * * `const char *multi_string_start`- a string that indicates the start of a
 *   multi line string.
 * * `const char *multi_string_end`- a string that indicates the end of a
 *   multi line string.
 * * `const char *word_chars` - Characters that are valid _word_ characters
 *   (for which `st_next_token()` returns `ST_WORD`) in addition to alphanumeric
 *   characters.
 * * `const char *operators` - Characters that should be treated as operators.
 *   If `st_next_token()` encounters any of these characters in the input stream,
 *   it will return the character verbatim.
 *
 * These members are used when a token has been read with `st_next_token()`:
 *
 * * `int token` - The last token that was read by `st_next_token()`.
 *   It will be one of the `st_token` values, or an ASCII character if
 *   the input matched one of the characters in `operators`
 * * `char value[]` - The string containing the value of the token (the _"lexeme"_)
 *   when a `ST_WORD`, `ST_STRING` or `ST_NUMBER` token has been read from the input
 *   stream. In the case of `ST_NUMBER`, the C standard `atoi()`, `atof()`, or
 *   `strtol()` functions can be used to convert it to the appropriate numeric
 *   type.
 *
 * These members are used when an error occurred in `st_next_token()` (in which
 * case `st_next_token()` will return `ST_ERROR`):
 *
 * * `int lineno` - The number of the line on which the error occurred.
 * * `const char *error_desc` -  a description of the error.
 */
typedef struct StrmTok {

    /* Determines where the data is read from */
    st_read_data_fun get_data;
    void *data;

    /* The internal buffer, where bytes are read into
    from the file, but before they're processed. */
    char raw_buffer[ST_READ_BUFFER_SIZE];
    int in_pos;

    int last_read;

    int unget_buf[ST_UNGET_COUNT];
    int unget_pos;

    int lineno;
    const char *error_desc;

    int lowercaseMode;
    int significantEol;

    const char *comment_chars;
    const char *single_comment;
    const char *multi_comment_start, *multi_comment_end;

    const char *quote_chars;
    const char *multi_string_start, *multi_string_end;
    const char *word_chars;
    const char *operators;

    int token;

    int p;
    char value[ST_BUFFER_SIZE];

} StrmTok;

/**
 * ## Functions
 *
 * ### `void st_init_custom(StrmTok *st, st_read_data_fun fun, void *data)`
 *
 * Initialises the `StrmTok` structure `st` to read tokens using a custom
 * `st_read_data_fun` function `fun`. `data` is passed unmodified to `fun`.
 */
_ST_EXPORT void st_init_custom(StrmTok *st, st_read_data_fun fun, void *data);

#ifdef EOF /* EOF will be defined if <stdio.h> is #included */

/**
 * ### `void st_init_file(StrmTok *st, FILE *file)`
 *
 * Initialises the `StrmTok` structure `st` to read tokens from a `FILE` object `file`.
 */
_ST_EXPORT void st_init_file(StrmTok *st, FILE *file);

/**
 * ### `void st_init_file_limit(StrmTok *st, struct st_read_limit *ll)`
 *
 * Initialises a `StrmTok` structure to read data from a file, but it will
 * only read a limited number of bytes from the file.
 *
 * (The intended use-case is where a file has been concatenated with other
 * files into an archive file)
 *
 * The `st_read_limit` structure `ll` is defined as follows:
 *
 * ```
 * struct st_read_limit {
 *   FILE *f;
 *   int limit;
 * };
 * ```
 *
 * where `f` is the file to read from and `limit` is the maximum
 * number of bytes that will be read from the file.
 */
struct st_read_limit {
    FILE *f;
    int limit;
};

_ST_EXPORT void st_init_file_limit(StrmTok *st, struct st_read_limit *ll);
#endif

/**
 * ### `void st_init_string(StrmTok *st, struct st_string_strm *strm)`
 *
 * Initialises the `StrmTok` structure `st` to read tokens from a null-terminated string.
 *
 * The `st_string_strm` structure `strm` has this definition:
 *
 * ```
 * struct st_string_strm {
 *  const char *str;
 *  unsigned int len;
 *  unsigned int p;
 * };
 * ```
 *
 * where
 *
 * * `str` is a pointer to the string.
 * * `len` is the length of the string.
 * * `p` is used internally to track the position.
 */
struct st_string_strm {
    const char *str;
    unsigned int len;
    unsigned int p;
};

_ST_EXPORT void st_init_string(StrmTok *st, struct st_string_strm *strm);

/**
 * ### `int st_next_token(StrmTok *st)`
 *
 * Reads the next token from the stream managed by the `StrmTok` structure `st`.
 *
 * It returns one of the `enum st_token` values, or an ASCII character
 * if the input matches one of the operators.
 */
_ST_EXPORT int st_next_token(StrmTok *st);

/* ==============================================================================
Implementation
============================================================================== */

#  if defined(STRMTOK_IMPLEMENTATION) || defined(STRMTOK_TEST)

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifdef __cplusplus
#  define CAST(x, y)   (x)y
#else
#  define CAST(x, y)   y
#endif

static int _st_get_char(StrmTok *st) {
    char c = 0;
    if(st->last_read == EOF) {
        return EOF;
    } else if(st->unget_pos > 0) {
        c = st->unget_buf[--st->unget_pos];
        st->last_read = c;
        return c;
    }
    if(st->in_pos >= ST_READ_BUFFER_SIZE || (c = st->raw_buffer[st->in_pos++]) == '\0') {
        int cnt = st->get_data(st->raw_buffer, ST_READ_BUFFER_SIZE - 1, st->data);
        if(!cnt) {
            st->last_read = EOF;
            return EOF;
        }
        st->in_pos = 0;
        c = st->raw_buffer[st->in_pos++];
    }
    return c;
}

static void _st_unget_char(StrmTok *st, int c) {
    assert(st->unget_pos < ST_UNGET_COUNT);
    st->unget_buf[st->unget_pos++] = c;
}

_ST_EXPORT void st_init_custom(StrmTok *st, st_read_data_fun fun, void *data) {
    st->get_data = fun;
    st->data = data;

    st->unget_pos = 0;

    st->in_pos = ST_READ_BUFFER_SIZE;
    st->token = ST_EOL;

    st->lineno = 1;
    st->error_desc = "no error";

    st->value[0] = '\0';
    st->p = 0;

    st->lowercaseMode = ST_DEFAULT_LOWERCASE_MODE;
    st->significantEol = ST_DEFAULT_SIGNIFICANT_EOL;
    st->comment_chars = ST_DEFAULT_COMMENT_CHARS;
    st->single_comment = ST_DEFAULT_SINGLE_COMMENT;
    st->multi_comment_start = ST_DEFAULT_MULTI_COMMENT_START;
    st->multi_comment_end = ST_DEFAULT_MULTI_COMMENT_END;
    st->quote_chars = ST_DEFAULT_QUOTE_CHARS;
    st->multi_string_start = ST_DEFAULT_MULTI_STRING_START;
    st->multi_string_end = ST_DEFAULT_MULTI_STRING_END;
    st->word_chars = ST_DEFAULT_WORD_CHARS;
    st->operators = ST_DEFAULT_OPERATORS;
}

static int _st_file_input_get_line(char *str, int num, void *data) {
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

_ST_EXPORT void st_init_file(StrmTok *st, FILE *file) {
    assert(file != NULL);
    st_init_custom(st, _st_file_input_get_line, file);
}

static int _st_file_input_get_line_limit(char *str, int num, void *data) {
    size_t read;
    struct st_read_limit *ll = CAST(struct st_read_limit *, data);
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

void st_init_file_limit(StrmTok *st, struct st_read_limit *ll) {
    assert(ll->f != NULL);
    assert(ll->limit > 0);
    st_init_custom(st, _st_file_input_get_line_limit, ll);
}

static int _st_string_input_get_line(char *str, int num, void *data) {
    struct st_string_strm *strm = CAST(struct st_string_strm *, data);

    if(strm->p >= strm->len)
        return 0;
    else if(strm->p + num > strm->len) {
        num = strm->len - strm->p;
    }

    memcpy(str, strm->str + strm->p, num);
    str[num] = '\0';

    strm->p += num;

    return 1;
}

_ST_EXPORT void st_init_string(StrmTok *st, struct st_string_strm *strm) {
    st_init_custom(st, _st_string_input_get_line, strm);
    strm->p = 0;
}

static int match_long_token(StrmTok *st, int c, const char *token) {
    if(!token[0]) {
        _st_unget_char(st, c);
        return 1;
    }
    if(c == token[0]) {
        c = _st_get_char(st);
        if(match_long_token(st, c, token + 1)) {
            return 1;
        } else {
            _st_unget_char(st, c);
        }
    }
    return 0;
}

static int _st_read_string_char(StrmTok *st, int c) {
    if(!c || c == EOF) {
        st->error_desc = "unterminated string constant";
        return 0;
    }
    if(st->p >= ST_BUFFER_SIZE) {
        st->error_desc = "token too long for value buffer";
        return 0;
    }
    if(c == '\\') {
        c = _st_get_char(st);
        if(!c || c == '\n' || c == EOF) {
            st->error_desc = "unterminated string constant";
            return 0;
        }
        switch(c) {
            case 'a' : st->value[st->p++] = '\a'; break;
            case 'b' : st->value[st->p++] = '\b'; break;
            case 'e' : st->value[st->p++] = 0x1B; break;
            case 'f' : st->value[st->p++] = '\f'; break;
            case 'n' : st->value[st->p++] = '\n'; break;
            case 'r' : st->value[st->p++] = '\r'; break;
            case 't' : st->value[st->p++] = '\t'; break;
            case 'v' : st->value[st->p++] = '\v'; break;
            default :  st->value[st->p++] = c; break;
        }
        /* TODO: JSON/JavaScript-style \uXXXX UTF-8 escapes (with surrogate pairs) */
    } else {
        st->value[st->p++] = c;
    }
    return 1;
}

#define ST_APPEND(c) do{st->value[st->p++] = (c); \
            if(st->p >= ST_BUFFER_SIZE) { \
                st->error_desc = "token too long for value buffer"; \
                goto error; \
            }} while(0)

_ST_EXPORT int st_next_token(StrmTok *st) {
    int c;
    const char *whitespace = st->significantEol ? " \t\r" : " \t\r\n";

    assert(st != NULL);

    if(st->token == ST_EOF || st->token == ST_ERROR)
        return st->token;

restart:
    do {
        c = _st_get_char(st);
        if(c == EOF)
            goto eof;
        if(c == '\n')
            st->lineno++;
    } while(strchr(whitespace, c));

    if((st->comment_chars && strchr(st->comment_chars, c))
        || (st->single_comment && match_long_token(st, c, st->single_comment))
    ) {
        do {
            c = _st_get_char(st);
            if(c == EOF)
                goto eof;
        } while(c != '\n');
        st->lineno++;
        goto restart;
    } else if(st->multi_comment_start && match_long_token(st, c, st->multi_comment_start)) {
        do {
            c = _st_get_char(st);
        } while(!match_long_token(st, c, st->multi_comment_end));
        goto restart;
    }

    if(st->significantEol && c == '\n') {
        st->token = ST_EOL;
    } else if(isalpha(c) || (st->word_chars && strchr(st->word_chars, c))) {

        st->value[0] = '\0';
        st->p = 0;

        do {
            ST_APPEND(st->lowercaseMode ? tolower(c) : c);
            c = _st_get_char(st);
        } while(isalnum(c) || (st->word_chars && strchr(st->word_chars, c)));
        _st_unget_char(st, c);
        st->value[st->p] = '\0';
        st->token = ST_WORD;

    } else if(isdigit(c)) {

        st->value[0] = '\0';
        st->p = 0;

        do {
            ST_APPEND(c);
            c = _st_get_char(st);
        } while(isdigit(c));

        if(c == '.') {
            ST_APPEND(c);
            c = _st_get_char(st);
            while(isdigit(c)) {
                ST_APPEND(c);
                c = _st_get_char(st);
            }
        }
        if(tolower(c) == 'e') {
            ST_APPEND(c);
            c = _st_get_char(st);
            if(strchr("+-", c)){
                ST_APPEND(c);
                c = _st_get_char(st);
            }
            while(isdigit(c)){
                ST_APPEND(c);
                c = _st_get_char(st);
            }
        }

        _st_unget_char(st, c);
        st->value[st->p] = '\0';
        st->token = ST_NUMBER;

    } else if(st->multi_string_start && match_long_token(st, c, st->multi_string_start)) {

        /* Why is BCC complaining here? */
        if(!st->multi_string_end)
            st->multi_string_end = st->multi_string_start;

        st->value[0] = '\0';
        st->p = 0;

        c = _st_get_char(st);
        while(!match_long_token(st, c, st->multi_string_end)) {
            if(!_st_read_string_char(st, c))
                goto error;
            c = _st_get_char(st);
        }
        st->value[st->p] = '\0';
        st->token = ST_STRING;

    } else if(st->quote_chars && strchr(st->quote_chars, c)) {
        int term = c;
        c = _st_get_char(st);

        st->value[0] = '\0';
        st->p = 0;

        while(c != term) {
            if(c == '\n') {
                st->error_desc = "unterminated string constant";
                goto error;
            }
            if(!_st_read_string_char(st, c))
                goto error;
            c = _st_get_char(st);
        }
        st->value[st->p] = '\0';
        st->token = ST_STRING;

    } else if(st->operators && strchr(st->operators, c)) {
        st->value[0] = c;
        st->value[1] = '\0';
        st->token = c;
    } else {
        st->error_desc = "unrecognised token";
        goto error;
    }
    return st->token;

eof:
    st->token = ST_EOF;
    return st->token;
error:
    st->token = ST_ERROR;
    return st->token;
}

#undef ST_APPEND

#  endif /* STRMTOK_IMPLEMENTATION */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* ==============================================================================
Test program.

Compile like so:
$ gcc -Wall -Wextra -std=c89 -D STRMTOK_TEST -xc strmtok.h
============================================================================== */

#ifdef STRMTOK_TEST

int main(int argc, char *argv[]) {

    StrmTok st;
    FILE *f = NULL;
    struct st_string_strm sts;

    if(argc > 1) {
        f = fopen(argv[1], "r");
        if(!f) {
            fprintf(stderr, "error: unable to open %s", argv[1]);
            return 1;
        }
#if 1
        st_init_file(&st, f);
#else
        struct st_read_limit ll;
        ll.f = f;
        ll.limit = 42;
        st_init_file_limit(&st, &ll);
#endif
    } else {
        sts.str = "1 foo 'a string' + bar_baz \n# a comment\n 1 2 fred";
        sts.len = strlen(sts.str);
        st_init_string(&st, &sts);
    }

    /*st.significantEol = 1;*/
    /*st.lowercaseMode = 1;*/
    st.comment_chars = "#;";
    st.operators = "[]+-:";
    st.word_chars = "$_";

    /*
    st.multi_comment_start = "--[[";
    st.multi_comment_end = "]]";
    */

    while(st_next_token(&st) != ST_EOF) {
        switch(st.token) {
            case ST_EOL:
                printf("EOL\n");
                break;
            case ST_WORD:
                printf("word ......: %s\n", st.value);
                break;
            case ST_STRING:
                printf("string ....: '%s'\n", st.value);
                break;
            case ST_NUMBER:
                printf("number ....: %s\n", st.value);
                break;
            case ST_ERROR:
                fprintf(stderr, "error: %d: %s\n", st.lineno, st.error_desc);
                return 1;
            default:
                printf("operator ..: '%c'\n", st.token);
                break;
        }
    }

    if(f)
        fclose(f);

    return 0;
}

#endif /* STRMTOK_TEST */

#endif /* STRMTOK_H */