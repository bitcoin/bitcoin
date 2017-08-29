/**
 * JSON Simple/Stacked/Stateful Lexer.
 * - Does not buffer data
 * - Maintains state
 * - Callback oriented
 * - Lightweight and fast. One source file and one header file
 *
 * Copyright (C) 2012-2015 Mark Nunberg
 * See included LICENSE file for license details.
 */

#ifndef JSONSL_H_
#define JSONSL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef JSONSL_USE_WCHAR
typedef jsonsl_char_t wchar_t;
typedef jsonsl_uchar_t unsigned wchar_t;
#else
typedef char jsonsl_char_t;
typedef unsigned char jsonsl_uchar_t;
#endif /* JSONSL_USE_WCHAR */

#ifdef JSONSL_PARSE_NAN
#define JSONSL__NAN_PROXY JSONSL_SPECIALf_NAN
#define JSONSL__INF_PROXY JSONSL_SPECIALf_INF
#else
#define JSONSL__NAN_PROXY 0
#define JSONSL__INF_PROXY 0
#endif

/* Stolen from http-parser.h, and possibly others */
#if defined(_WIN32) && !defined(__MINGW32__) && (!defined(_MSC_VER) || _MSC_VER<1600)
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#if !defined(_MSC_VER) || _MSC_VER<1400
typedef unsigned int size_t;
typedef int ssize_t;
#endif
#else
#include <stdint.h>
#endif


#if (!defined(JSONSL_STATE_GENERIC)) && (!defined(JSONSL_STATE_USER_FIELDS))
#define JSONSL_STATE_GENERIC
#endif /* !defined JSONSL_STATE_GENERIC */

#ifdef JSONSL_STATE_GENERIC
#define JSONSL_STATE_USER_FIELDS
#endif /* JSONSL_STATE_GENERIC */

/* Additional fields for component object */
#ifndef JSONSL_JPR_COMPONENT_USER_FIELDS
#define JSONSL_JPR_COMPONENT_USER_FIELDS
#endif

#ifndef JSONSL_API
/**
 * We require a /DJSONSL_DLL so that users already using this as a static
 * or embedded library don't get confused
 */
#if defined(_WIN32) && defined(JSONSL_DLL)
#define JSONSL_API __declspec(dllexport)
#else
#define JSONSL_API
#endif /* _WIN32 */

#endif /* !JSONSL_API */

#ifndef JSONSL_INLINE
#if defined(_MSC_VER)
  #define JSONSL_INLINE __inline
  #elif defined(__GNUC__)
  #define JSONSL_INLINE __inline__
  #else
  #define JSONSL_INLINE inline
  #endif /* _MSC_VER or __GNUC__ */
#endif /* JSONSL_INLINE */

#define JSONSL_MAX_LEVELS 512

struct jsonsl_st;
typedef struct jsonsl_st *jsonsl_t;

typedef struct jsonsl_jpr_st* jsonsl_jpr_t;

/**
 * This flag is true when AND'd against a type whose value
 * must be in "quoutes" i.e. T_HKEY and T_STRING
 */
#define JSONSL_Tf_STRINGY 0xffff00

/**
 * Constant representing the special JSON types.
 * The values are special and aid in speed (the OBJECT and LIST
 * values are the char literals of their openings).
 *
 * Their actual value is a character which attempts to resemble
 * some mnemonic reference to the actual type.
 *
 * If new types are added, they must fit into the ASCII printable
 * range (so they should be AND'd with 0x7f and yield something
 * meaningful)
 */
#define JSONSL_XTYPE \
    X(STRING,   '"'|JSONSL_Tf_STRINGY) \
    X(HKEY,     '#'|JSONSL_Tf_STRINGY) \
    X(OBJECT,   '{') \
    X(LIST,     '[') \
    X(SPECIAL,  '^') \
    X(UESCAPE,  'u')
typedef enum {
#define X(o, c) \
    JSONSL_T_##o = c,
    JSONSL_XTYPE
    JSONSL_T_UNKNOWN = '?',
    /* Abstract 'root' object */
    JSONSL_T_ROOT = 0
#undef X
} jsonsl_type_t;

/**
 * Subtypes for T_SPECIAL. We define them as flags
 * because more than one type can be applied to a
 * given object.
 */

#define JSONSL_XSPECIAL \
    X(NONE, 0) \
    X(SIGNED,       1<<0) \
    X(UNSIGNED,     1<<1) \
    X(TRUE,         1<<2) \
    X(FALSE,        1<<3) \
    X(NULL,         1<<4) \
    X(FLOAT,        1<<5) \
    X(EXPONENT,     1<<6) \
    X(NONASCII,     1<<7) \
    X(NAN,          1<<8) \
    X(INF,          1<<9)
typedef enum {
#define X(o,b) \
    JSONSL_SPECIALf_##o = b,
    JSONSL_XSPECIAL
#undef X
    /* Handy flags for checking */

    JSONSL_SPECIALf_UNKNOWN = 1 << 10,

    /** @private Private */
    JSONSL_SPECIALf_ZERO    = 1 << 11 | JSONSL_SPECIALf_UNSIGNED,
    /** @private */
    JSONSL_SPECIALf_DASH    = 1 << 12,
    /** @private */
    JSONSL_SPECIALf_POS_INF = (JSONSL_SPECIALf_INF),
    JSONSL_SPECIALf_NEG_INF = (JSONSL_SPECIALf_INF|JSONSL_SPECIALf_SIGNED),

    /** Type is numeric */
    JSONSL_SPECIALf_NUMERIC = (JSONSL_SPECIALf_SIGNED| JSONSL_SPECIALf_UNSIGNED),

    /** Type is a boolean */
    JSONSL_SPECIALf_BOOLEAN = (JSONSL_SPECIALf_TRUE|JSONSL_SPECIALf_FALSE),

    /** Type is an "extended", not integral type (but numeric) */
   JSONSL_SPECIALf_NUMNOINT =
       (JSONSL_SPECIALf_FLOAT|JSONSL_SPECIALf_EXPONENT|JSONSL_SPECIALf_NAN
        |JSONSL_SPECIALf_INF)
} jsonsl_special_t;


/**
 * These are the various types of stack (or other) events
 * which will trigger a callback.
 * Like the type constants, this are also mnemonic
 */
#define JSONSL_XACTION \
    X(PUSH, '+') \
    X(POP, '-') \
    X(UESCAPE, 'U') \
    X(ERROR, '!')
typedef enum {
#define X(a,c) \
    JSONSL_ACTION_##a = c,
    JSONSL_XACTION
    JSONSL_ACTION_UNKNOWN = '?'
#undef X
} jsonsl_action_t;


/**
 * Various errors which may be thrown while parsing JSON
 */
#define JSONSL_XERR \
/* Trailing garbage characters */ \
    X(GARBAGE_TRAILING) \
/* We were expecting a 'special' (numeric, true, false, null) */ \
    X(SPECIAL_EXPECTED) \
/* The 'special' value was incomplete */ \
    X(SPECIAL_INCOMPLETE) \
/* Found a stray token */ \
    X(STRAY_TOKEN) \
/* We were expecting a token before this one */ \
    X(MISSING_TOKEN) \
/* Cannot insert because the container is not ready */ \
    X(CANT_INSERT) \
/* Found a '\' outside a string */ \
    X(ESCAPE_OUTSIDE_STRING) \
/* Found a ':' outside of a hash */ \
    X(KEY_OUTSIDE_OBJECT) \
/* found a string outside of a container */ \
    X(STRING_OUTSIDE_CONTAINER) \
/* Found a null byte in middle of string */ \
    X(FOUND_NULL_BYTE) \
/* Current level exceeds limit specified in constructor */ \
    X(LEVELS_EXCEEDED) \
/* Got a } as a result of an opening [ or vice versa */ \
    X(BRACKET_MISMATCH) \
/* We expected a key, but got something else instead */ \
    X(HKEY_EXPECTED) \
/* We got an illegal control character (bad whitespace or something) */ \
    X(WEIRD_WHITESPACE) \
/* Found a \u-escape, but there were less than 4 following hex digits */ \
    X(UESCAPE_TOOSHORT) \
/* Invalid two-character escape */ \
    X(ESCAPE_INVALID) \
/* Trailing comma */ \
    X(TRAILING_COMMA) \
/* An invalid number was passed in a numeric field */ \
    X(INVALID_NUMBER) \
/* Value is missing for object */ \
    X(VALUE_EXPECTED) \
/* The following are for JPR Stuff */ \
    \
/* Found a literal '%' but it was only followed by a single valid hex digit */ \
    X(PERCENT_BADHEX) \
/* jsonpointer URI is malformed '/' */ \
    X(JPR_BADPATH) \
/* Duplicate slash */ \
    X(JPR_DUPSLASH) \
/* No leading root */ \
    X(JPR_NOROOT) \
/* Allocation failure */ \
    X(ENOMEM) \
/* Invalid unicode codepoint detected (in case of escapes) */ \
    X(INVALID_CODEPOINT)

typedef enum {
    JSONSL_ERROR_SUCCESS = 0,
#define X(e) \
    JSONSL_ERROR_##e,
    JSONSL_XERR
#undef X
    JSONSL_ERROR_GENERIC
} jsonsl_error_t;


/**
 * A state is a single level of the stack.
 * Non-private data (i.e. the 'data' field, see the STATE_GENERIC section)
 * will remain in tact until the item is popped.
 *
 * As a result, it means a parent state object may be accessed from a child
 * object, (the parents fields will all be valid). This allows a user to create
 * an ad-hoc hierarchy on top of the JSON one.
 *
 */
struct jsonsl_state_st {
    /**
     * The JSON object type
     */
    unsigned type;

    /** If this element is special, then its extended type is here */
    unsigned special_flags;

    /**
     * The position (in terms of number of bytes since the first call to
     * jsonsl_feed()) at which the state was first pushed. This includes
     * opening tokens, if applicable.
     *
     * @note For strings (i.e. type & JSONSL_Tf_STRINGY is nonzero) this will
     * be the position of the first quote.
     *
     * @see jsonsl_st::pos which contains the _current_ position and can be
     * used during a POP callback to get the length of the element.
     */
    size_t pos_begin;

    /**FIXME: This is redundant as the same information can be derived from
     * jsonsl_st::pos at pop-time */
    size_t pos_cur;

    /**
     * Level of recursion into nesting. This is mainly a convenience
     * variable, as this can technically be deduced from the lexer's
     * level parameter (though the logic is not that simple)
     */
    unsigned int level;


    /**
     * how many elements in the object/list.
     * For objects (hashes), an element is either
     * a key or a value. Thus for one complete pair,
     * nelem will be 2.
     *
     * For special types, this will hold the sum of the digits.
     * This only holds true for values which are simple signed/unsigned
     * numbers. Otherwise a special flag is set, and extra handling is not
     * performed.
     */
    uint64_t nelem;



    /*TODO: merge this and special_flags into a union */


    /**
     * Useful for an opening nest, this will prevent a callback from being
     * invoked on this item or any of its children
     */
    int ignore_callback;

    /**
     * Counter which is incremented each time an escape ('\') is encountered.
     * This is used internally for non-string types and should only be
     * inspected by the user if the state actually represents a string
     * type.
     */
    unsigned int nescapes;

    /**
     * Put anything you want here. if JSONSL_STATE_USER_FIELDS is here, then
     * the macro expansion happens here.
     *
     * You can use these fields to store hierarchical or 'tagging' information
     * for specific objects.
     *
     * See the documentation above for the lifetime of the state object (i.e.
     * if the private data points to allocated memory, it should be freed
     * when the object is popped, as the state object will be re-used)
     */
#ifndef JSONSL_STATE_GENERIC
    JSONSL_STATE_USER_FIELDS
#else

    /**
     * Otherwise, this is a simple void * pointer for anything you want
     */
    void *data;
#endif /* JSONSL_STATE_USER_FIELDS */
};

/**Gets the number of elements in the list.
 * @param st The state. Must be of type JSONSL_T_LIST
 * @return number of elements in the list
 */
#define JSONSL_LIST_SIZE(st) ((st)->nelem)

/**Gets the number of key-value pairs in an object
 * @param st The state. Must be of type JSONSL_T_OBJECT
 * @return the number of key-value pairs in the object
 */
#define JSONSL_OBJECT_SIZE(st) ((st)->nelem / 2)

/**Gets the numeric value.
 * @param st The state. Must be of type JSONSL_T_SPECIAL and
 *           special_flags must have the JSONSL_SPECIALf_NUMERIC flag
 *           set.
 * @return the numeric value of the state.
 */
#define JSONSL_NUMERIC_VALUE(st) ((st)->nelem)

/*
 * So now we need some special structure for keeping the
 * JPR info in sync. Preferrably all in a single block
 * of memory (there's no need for separate allocations.
 * So we will define a 'table' with the following layout
 *
 * Level    nPosbl  JPR1_last   JPR2_last   JPR3_last
 *
 * 0        1       NOMATCH     POSSIBLE    POSSIBLE
 * 1        0       NOMATCH     NOMATCH     COMPLETE
 * [ table ends here because no further path is possible]
 *
 * Where the JPR..n corresponds to the number of JPRs
 * requested, and nPosble is a quick flag to determine
 *
 * the number of possibilities. In the future this might
 * be made into a proper 'jump' table,
 *
 * Since we always mark JPRs from the higher levels descending
 * into the lower ones, a prospective child match would first
 * look at the parent table to check the possibilities, and then
 * see which ones were possible..
 *
 * Thus, the size of this blob would be (and these are all ints here)
 * nLevels * nJPR * 2.
 *
 * the 'Width' of the table would be nJPR*2, and the 'height' would be
 * nlevels
 */

/**
 * This is called when a stack change ocurs.
 *
 * @param jsn The lexer
 * @param action The type of action, this can be PUSH or POP
 * @param state A pointer to the stack currently affected by the action
 * @param at A pointer to the position of the input buffer which triggered
 * this action.
 */
typedef void (*jsonsl_stack_callback)(
        jsonsl_t jsn,
        jsonsl_action_t action,
        struct jsonsl_state_st* state,
        const jsonsl_char_t *at);


/**
 * This is called when an error is encountered.
 * Sometimes it's possible to 'erase' characters (by replacing them
 * with whitespace). If you think you have corrected the error, you
 * can return a true value, in which case the parser will backtrack
 * and try again.
 *
 * @param jsn The lexer
 * @param error The error which was thrown
 * @param state the current state
 * @param a pointer to the position of the input buffer which triggered
 * the error. Note that this is not const, this is because you have the
 * possibility of modifying the character in an attempt to correct the
 * error
 *
 * @return zero to bail, nonzero to try again (this only makes sense if
 * the input buffer has been modified by this callback)
 */
typedef int (*jsonsl_error_callback)(
        jsonsl_t jsn,
        jsonsl_error_t error,
        struct jsonsl_state_st* state,
        jsonsl_char_t *at);

struct jsonsl_st {
    /** Public, read-only */

    /** This is the current level of the stack */
    unsigned int level;

    /** Flag set to indicate we should stop processing */
    unsigned int stopfl;

    /**
     * This is the current position, relative to the beginning
     * of the stream.
     */
    size_t pos;

    /** This is the 'bytes' variable passed to feed() */
    const jsonsl_char_t *base;

    /** Callback invoked for PUSH actions */
    jsonsl_stack_callback action_callback_PUSH;

    /** Callback invoked for POP actions */
    jsonsl_stack_callback action_callback_POP;

    /** Default callback for any action, if neither PUSH or POP callbacks are defined */
    jsonsl_stack_callback action_callback;

    /**
     * Do not invoke callbacks for objects deeper than this level.
     * NOTE: This field establishes the lower bound for ignored callbacks,
     * and is thus misnamed. `min_ignore_level` would actually make more
     * sense, but we don't want to break API.
     */
    unsigned int max_callback_level;

    /** The error callback. Invoked when an error happens. Should not be NULL */
    jsonsl_error_callback error_callback;

    /* these are boolean flags you can modify. You will be called
     * about notification for each of these types if the corresponding
     * variable is true.
     */

    /**
     * @name Callback Booleans.
     * These determine whether a callback is to be invoked for certain types of objects
     * @{*/

    /** Boolean flag to enable or disable the invokcation for events on this type*/
    int call_SPECIAL;
    int call_OBJECT;
    int call_LIST;
    int call_STRING;
    int call_HKEY;
    /*@}*/

    /**
     * @name u-Escape handling
     * Special handling for the \\u-f00d type sequences. These are meant
     * to be translated back into the corresponding octet(s).
     * A special callback (if set) is invoked with *at=='u'. An application
     * may wish to temporarily suspend parsing and handle the 'u-' sequence
     * internally (or not).
     */

     /*@{*/

    /** Callback to be invoked for a u-escape */
    jsonsl_stack_callback action_callback_UESCAPE;

    /** Boolean flag, whether to invoke the callback */
    int call_UESCAPE;

    /** Boolean flag, whether we should return after encountering a u-escape:
     * the callback is invoked and then we return if this is true
     */
    int return_UESCAPE;
    /*@}*/

    struct {
        int allow_trailing_comma;
    } options;

    /** Put anything here */
    void *data;

    /*@{*/
    /** Private */
    int in_escape;
    char expecting;
    char tok_last;
    int can_insert;
    unsigned int levels_max;

#ifndef JSONSL_NO_JPR
    size_t jpr_count;
    jsonsl_jpr_t *jprs;

    /* Root pointer for JPR matching information */
    size_t *jpr_root;
#endif /* JSONSL_NO_JPR */
    /*@}*/

    /**
     * This is the stack. Its upper bound is levels_max, or the
     * nlevels argument passed to jsonsl_new. If you modify this structure,
     * make sure that this member is last.
     */
    struct jsonsl_state_st stack[1];
};


/**
 * Creates a new lexer object, with capacity for recursion up to nlevels
 *
 * @param nlevels maximum recursion depth
 */
JSONSL_API
jsonsl_t jsonsl_new(int nlevels);

/**
 * Feeds data into the lexer.
 *
 * @param jsn the lexer object
 * @param bytes new data to be fed
 * @param nbytes size of new data
 */
JSONSL_API
void jsonsl_feed(jsonsl_t jsn, const jsonsl_char_t *bytes, size_t nbytes);

/**
 * Resets the internal parser state. This does not free the parser
 * but does clean it internally, so that the next time feed() is called,
 * it will be treated as a new stream
 *
 * @param jsn the lexer
 */
JSONSL_API
void jsonsl_reset(jsonsl_t jsn);

/**
 * Frees the lexer, cleaning any allocated memory taken
 *
 * @param jsn the lexer
 */
JSONSL_API
void jsonsl_destroy(jsonsl_t jsn);

/**
 * Gets the 'parent' element, given the current one
 *
 * @param jsn the lexer
 * @param cur the current nest, which should be a struct jsonsl_nest_st
 */
static JSONSL_INLINE
struct jsonsl_state_st *jsonsl_last_state(const jsonsl_t jsn,
                                          const struct jsonsl_state_st *state)
{
    /* Don't complain about overriding array bounds */
    if (state->level > 1) {
        return jsn->stack + state->level - 1;
    } else {
        return NULL;
    }
}

/**
 * Gets the state of the last fully consumed child of this parent. This is
 * only valid in the parent's POP callback.
 *
 * @param the lexer
 * @return A pointer to the child.
 */
static JSONSL_INLINE
struct jsonsl_state_st *jsonsl_last_child(const jsonsl_t jsn,
                                          const struct jsonsl_state_st *parent)
{
    return jsn->stack + (parent->level + 1);
}

/**Call to instruct the parser to stop parsing and return. This is valid
 * only from within a callback */
static JSONSL_INLINE
void jsonsl_stop(jsonsl_t jsn)
{
    jsn->stopfl = 1;
}

/**
 * This enables receiving callbacks on all events. Doesn't do
 * anything special but helps avoid some boilerplate.
 * This does not touch the UESCAPE callbacks or flags.
 */
static JSONSL_INLINE
void jsonsl_enable_all_callbacks(jsonsl_t jsn)
{
    jsn->call_HKEY = 1;
    jsn->call_STRING = 1;
    jsn->call_OBJECT = 1;
    jsn->call_SPECIAL = 1;
    jsn->call_LIST = 1;
}

/**
 * A macro which returns true if the current state object can
 * have children. This means a list type or an object type.
 */
#define JSONSL_STATE_IS_CONTAINER(state) \
        (state->type == JSONSL_T_OBJECT || state->type == JSONSL_T_LIST)

/**
 * These two functions, dump a string representation
 * of the error or type, respectively. They will never
 * return NULL
 */
JSONSL_API
const char* jsonsl_strerror(jsonsl_error_t err);
JSONSL_API
const char* jsonsl_strtype(jsonsl_type_t jt);

/**
 * Dumps global metrics to the screen. This is a noop unless
 * jsonsl was compiled with JSONSL_USE_METRICS
 */
JSONSL_API
void jsonsl_dump_global_metrics(void);

/* This macro just here for editors to do code folding */
#ifndef JSONSL_NO_JPR

/**
 * @name JSON Pointer API
 *
 * JSONPointer API. This isn't really related to the lexer (at least not yet)
 * JSONPointer provides an extremely simple specification for providing
 * locations within JSON objects. We will extend it a bit and allow for
 * providing 'wildcard' characters by which to be able to 'query' the stream.
 *
 * See http://tools.ietf.org/html/draft-pbryan-zyp-json-pointer-00
 *
 * Currently I'm implementing the 'single query' API which can only use a single
 * query component. In the future I will integrate my yet-to-be-published
 * Boyer-Moore-esque prefix searching implementation, in order to allow
 * multiple paths to be merged into one for quick and efficient searching.
 *
 *
 * JPR (as we'll refer to it within the source) can be used by splitting
 * the components into mutliple sections, and incrementally 'track' each
 * component. When JSONSL delivers a 'pop' callback for a string, or a 'push'
 * callback for an object, we will check to see whether the index matching
 * the component corresponding to the current level contains a match
 * for our path.
 *
 * In order to do this properly, a structure must be maintained within the
 * parent indicating whether its children are possible matches. This flag
 * will be 'inherited' by call children which may conform to the match
 * specification, and discarded by all which do not (thereby eliminating
 * their children from inheriting it).
 *
 * A successful match is a complete one. One can provide multiple paths with
 * multiple levels of matches e.g.
 *  /foo/bar/baz/^/blah
 *
 *  @{
 */

/** The wildcard character */
#ifndef JSONSL_PATH_WILDCARD_CHAR
#define JSONSL_PATH_WILDCARD_CHAR '^'
#endif /* WILDCARD_CHAR */

#define JSONSL_XMATCH \
    X(COMPLETE,1) \
    X(POSSIBLE,0) \
    X(NOMATCH,-1) \
    X(TYPE_MISMATCH, -2)

typedef enum {

#define X(T,v) \
    JSONSL_MATCH_##T = v,
    JSONSL_XMATCH

#undef X
    JSONSL_MATCH_UNKNOWN
} jsonsl_jpr_match_t;

typedef enum {
    JSONSL_PATH_STRING = 1,
    JSONSL_PATH_WILDCARD,
    JSONSL_PATH_NUMERIC,
    JSONSL_PATH_ROOT,

    /* Special */
    JSONSL_PATH_INVALID = -1,
    JSONSL_PATH_NONE = 0
} jsonsl_jpr_type_t;

struct jsonsl_jpr_component_st {
    /** The string the component points to */
    char *pstr;
    /** if this is a numeric type, the number is 'cached' here */
    unsigned long idx;
    /** The length of the string */
    size_t len;
    /** The type of component (NUMERIC or STRING) */
    jsonsl_jpr_type_t ptype;

    /** Set this to true to enforce type checking between dict keys and array
     * indices. jsonsl_jpr_match() will return TYPE_MISMATCH if it detects
     * that an array index is actually a child of a dictionary. */
    short is_arridx;

    /* Extra fields (for more advanced searches. Default is empty) */
    JSONSL_JPR_COMPONENT_USER_FIELDS
};

struct jsonsl_jpr_st {
    /** Path components */
    struct jsonsl_jpr_component_st *components;
    size_t ncomponents;

    /**Type of the match to be expected. If nonzero, will be compared against
     * the actual type */
    unsigned match_type;

    /** Base of allocated string for components */
    char *basestr;

    /** The original match string. Useful for returning to the user */
    char *orig;
    size_t norig;
};

/**
 * Create a new JPR object.
 *
 * @param path the JSONPointer path specification.
 * @param errp a pointer to a jsonsl_error_t. If this function returns NULL,
 * then more details will be in this variable.
 *
 * @return a new jsonsl_jpr_t object, or NULL on error.
 */
JSONSL_API
jsonsl_jpr_t jsonsl_jpr_new(const char *path, jsonsl_error_t *errp);

/**
 * Destroy a JPR object
 */
JSONSL_API
void jsonsl_jpr_destroy(jsonsl_jpr_t jpr);

/**
 * Match a JSON object against a type and specific level
 *
 * @param jpr the JPR object
 * @param parent_type the type of the parent (should be T_LIST or T_OBJECT)
 * @param parent_level the level of the parent
 * @param key the 'key' of the child. If the parent is an array, this should be
 * empty.
 * @param nkey - the length of the key. If the parent is an array (T_LIST), then
 * this should be the current index.
 *
 * NOTE: The key of the child means any kind of associative data related to the
 * element. Thus: <<< { "foo" : [ >>,
 * the opening array's key is "foo".
 *
 * @return a status constant. This indicates whether a match was excluded, possible,
 * or successful.
 */
JSONSL_API
jsonsl_jpr_match_t jsonsl_jpr_match(jsonsl_jpr_t jpr,
                                    unsigned int parent_type,
                                    unsigned int parent_level,
                                    const char *key, size_t nkey);

/**
 * Alternate matching algorithm. This matching algorithm does not use
 * JSONPointer but relies on a more structured searching mechanism. It
 * assumes that there is a clear distinction between array indices and
 * object keys. In this case, the jsonsl_path_component_st::ptype should
 * be set to @ref JSONSL_PATH_NUMERIC for an array index (the
 * jsonsl_path_comonent_st::is_arridx field will be removed in a future
 * version).
 *
 * @param jpr The path
 * @param parent The parent structure. Can be NULL if this is the root object
 * @param child The child structure. Should not be NULL
 * @param key Object key, if an object
 * @param nkey Length of object key
 * @return Status constant if successful
 *
 * @note
 * For successful matching, both the key and the path itself should be normalized
 * to contain 'proper' utf8 sequences rather than utf16 '\uXXXX' escapes. This
 * should currently be done in the application. Another version of this function
 * may use a temporary buffer in such circumstances (allocated by the application).
 *
 * Since this function also checks the state of the child, it should only
 * be called on PUSH callbacks, and not POP callbacks
 */
JSONSL_API
jsonsl_jpr_match_t
jsonsl_path_match(jsonsl_jpr_t jpr,
                  const struct jsonsl_state_st *parent,
                  const struct jsonsl_state_st *child,
                  const char *key, size_t nkey);


/**
 * Associate a set of JPR objects with a lexer instance.
 * This should be called before the lexer has been fed any data (and
 * behavior is undefined if you don't adhere to this).
 *
 * After using this function, you may subsequently call match_state() on
 * given states (presumably from within the callbacks).
 *
 * Note that currently the first JPR is the quickest and comes
 * pre-allocated with the state structure. Further JPR objects
 * are chained.
 *
 * @param jsn The lexer
 * @param jprs An array of jsonsl_jpr_t objects
 * @param njprs How many elements in the jprs array.
 */
JSONSL_API
void jsonsl_jpr_match_state_init(jsonsl_t jsn,
                                 jsonsl_jpr_t *jprs,
                                 size_t njprs);

/**
 * This follows the same semantics as the normal match,
 * except we infer parent and type information from the relevant state objects.
 * The match status (for all possible JPR objects) is set in the *out parameter.
 *
 * If a match has succeeded, then its JPR object will be returned. In all other
 * instances, NULL is returned;
 *
 * @param jpr The jsonsl_jpr_t handle
 * @param state The jsonsl_state_st which is a candidate
 * @param key The hash key (if applicable, can be NULL if parent is list)
 * @param nkey Length of hash key (if applicable, can be zero if parent is list)
 * @param out A pointer to a jsonsl_jpr_match_t. This will be populated with
 * the match result
 *
 * @return If a match was completed in full, then the JPR object containing
 * the matching path will be returned. Otherwise, the return is NULL (note, this
 * does not mean matching has failed, it can still be part of the match: check
 * the out parameter).
 */
JSONSL_API
jsonsl_jpr_t jsonsl_jpr_match_state(jsonsl_t jsn,
                                    struct jsonsl_state_st *state,
                                    const char *key,
                                    size_t nkey,
                                    jsonsl_jpr_match_t *out);


/**
 * Cleanup any memory allocated and any states set by
 * match_state_init() and match_state()
 * @param jsn The lexer
 */
JSONSL_API
void jsonsl_jpr_match_state_cleanup(jsonsl_t jsn);

/**
 * Return a string representation of the match result returned by match()
 */
JSONSL_API
const char *jsonsl_strmatchtype(jsonsl_jpr_match_t match);

/* @}*/

/**
 * Utility function to convert escape sequences into their original form.
 *
 * The decoders I've sampled do not seem to specify a standard behavior of what
 * to escape/unescape.
 *
 * RFC 4627 Mandates only that the quoute, backslash, and ASCII control
 * characters (0x00-0x1f) be escaped. It is often common for applications
 * to escape a '/' - however this may also be desired behavior. the JSON
 * spec is not clear on this, and therefore jsonsl leaves it up to you.
 *
 * Additionally, sometimes you may wish to _normalize_ JSON. This is specifically
 * true when dealing with 'u-escapes' which can be expressed perfectly fine
 * as utf8. One use case for normalization is JPR string comparison, in which
 * case two effectively equivalent strings may not match because one is using
 * u-escapes and the other proper utf8. To normalize u-escapes only, pass in
 * an empty `toEscape` table, enabling only the `u` index.
 *
 * @param in The input string.
 * @param out An allocated output (should be the same size as in)
 * @param len the size of the buffer
 * @param toEscape - A sparse array of characters to unescape. Characters
 * which are not present in this array, e.g. toEscape['c'] == 0 will be
 * ignored and passed to the output in their original form.
 * @param oflags If not null, and a \uXXXX escape expands to a non-ascii byte,
 * then this variable will have the SPECIALf_NONASCII flag on.
 *
 * @param err A pointer to an error variable. If an error ocurrs, it will be
 * set in this variable
 * @param errat If not null and an error occurs, this will be set to point
 * to the position within the string at which the offending character was
 * encountered.
 *
 * @return The effective size of the output buffer.
 *
 * @note
 * This function now encodes the UTF8 equivalents of utf16 escapes (i.e.
 * 'u-escapes'). Previously this would encode the escapes as utf16 literals,
 * which while still correct in some sense was confusing for many (especially
 * considering that the inputs were variations of char).
 *
 * @note
 * The output buffer will never be larger than the input buffer, since
 * standard escape sequences (i.e. '\t') occupy two bytes in the source
 * but only one byte (when unescaped) in the output. Likewise u-escapes
 * (i.e. \uXXXX) will occupy six bytes in the source, but at the most
 * two bytes when escaped.
 */
JSONSL_API
size_t jsonsl_util_unescape_ex(const char *in,
                               char *out,
                               size_t len,
                               const int toEscape[128],
                               unsigned *oflags,
                               jsonsl_error_t *err,
                               const char **errat);

/**
 * Convenience macro to avoid passing too many parameters
 */
#define jsonsl_util_unescape(in, out, len, toEscape, err) \
    jsonsl_util_unescape_ex(in, out, len, toEscape, NULL, err, NULL)

#endif /* JSONSL_NO_JPR */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* JSONSL_H_ */
