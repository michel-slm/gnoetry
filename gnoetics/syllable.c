/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* 
   This is based on the algorithm in Greg Fast's perl module
   Lingua::EN::Syllable.
*/

#include <Python.h>
#include <glib.h>

#include "syllable.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define SYLLABLEFILE "syllables"

static const char *subsyl[] = {
    "cial", "tia", "cius", "cious", "giu", "ion", "iou", "sia$", ".ely$",
    NULL
};

/* 
   ^ == match beginning
   . == match anything
   $ == match end
   ! == not next match
   + == match aeiouy
   _ == match aeiou
   < == match prev char
   @ == match g or q.  an awful hack
*/
   
static const char *addsyl[] = {
    "ia", "riet", "dien", "iu", "io", "ii",
    "+bl$", "mbl$",
    "___",
    "^mc", "ism$",
    "!+<l$",
    "!llien",
    "^coad.", "^coag.", "^coal.", "^coax.",
    "!@ua!_",
    "dnt$",
    NULL
};

static gboolean 
is_vowel (char c)
{
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

static gboolean 
is_vowely (char c)
{
    return is_vowel (c) || c == 'y';
}


static gboolean
baby_regexp_match (const char *str, const char *regexp)
{
    const char *s;
    const char *ss;
    const char *p;
    

    for (s=str; *s; ++s) {

        gboolean match = TRUE;
        ss = s;
        p = regexp;

        if (*p == '^' && s != str)
            match = FALSE;

        while (match && *ss && *p) {


            if (*p != '.') {

                gboolean flip = FALSE;

                if (*p == '!' && *(p+1) != '\0') {
                    flip = TRUE;
                    ++p;
                }

                if (*p == '+')
                    match = match && is_vowely (*ss);
                else if (*p == '_')
                    match = match && is_vowel (*ss);
                else if (*p == '<')
                    match = match &&  (ss != str && *ss == *(ss-1));
                else if (*p == '@')
                    match = match && (*ss == 'g' || *ss == 'q');
                else if (*p != *ss)
                    match = FALSE;

                if (flip)
                    match = !match;
	
            }

            ++p;
            ++ss;
        }

        if (match && *ss == '\0' && *p != '\0' && *p != '$')
            match = FALSE;

        if (match)
            return TRUE;
    }

    return FALSE;
}

/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

static GHashTable *syllable_override_table = NULL;

static const char *default_syllable_paths[] = {
#ifdef SYLLABLEFILE
#ifdef DICTPATH
    DICTPATH "/" SYLLABLEFILE,
#endif  
    "dict/" SYLLABLEFILE,
    "../dict/" SYLLABLEFILE,
    "../../dict/" SYLLABLEFILE,
    "../../../dict/" SYLLABLEFILE,
#endif
    NULL
};

static void
load_syllable_override_table (void)
{
    FILE *in;
    char buffer[512];
    int i;

    if (syllable_override_table)
        return;

    syllable_override_table = g_hash_table_new (g_str_hash, g_str_equal);

    i = 0;
    in = NULL;
    while (default_syllable_paths[i] && in == NULL) {
        in = fopen (default_syllable_paths[i], "r");
        ++i;
    }

    if (in == NULL) {
        g_warning ("Can't open syllable override dictionary.");
        return;
    }

    while (fgets (buffer, 512, in)) {

        if (buffer[0] && buffer[0] != '#') {
            char word[512];
            int syl;
            if (sscanf (buffer, "%s %d", word, &syl) == 2) {
	
                g_strdup (word);

                if (g_hash_table_lookup (syllable_override_table, word))
                    g_warning ("Duplicate syllable override value for %s", word);

                g_hash_table_insert (syllable_override_table,
                                     g_strdup (word),
                                     GINT_TO_POINTER (syl));
            }
        }

    }
  
    fclose (in);
}

/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

int
syllable_count_approximate (const char *str)
{
    int count=0;
    const char *s;
    int i;
    gboolean prev_was_vowel=FALSE;
    char *cpy;
    char *c;

    g_return_val_if_fail (str, -1);

    cpy = g_strdup (str);
    g_strdown (cpy);

    for (i=0; cpy[i]; ++i) 
        if (cpy[i] == '.' 
            || cpy[i] == '-'
            || cpy[i] == ','
            || cpy[i] == '|'
            || cpy[i] == ':' 
            || cpy[i] == ';' 
            || cpy[i] == '!'
            || cpy[i] == '?')
            cpy[i] = ' ';



    /* Handle special exceptions */

    if (syllable_override_table == NULL) 
        load_syllable_override_table ();

    if (syllable_override_table) {
        gpointer excep = g_hash_table_lookup (syllable_override_table, cpy);
        if (excep) {
            g_free (cpy);
            return GPOINTER_TO_INT (excep);
        }
    }

  
    /* If our "word" contains whitespace, split it apart and deal with each
       bit separately. */
    for (i=0; cpy[i]; ++i) {
        if (isspace (cpy[i])) {
            int sum = 0;

            char *a = cpy;
            char *b = cpy;
            gboolean done = FALSE;

            while (*b) {

                while (*b && !isspace (*b))
                    ++b;

                if (*b == '\0')
                    done = TRUE;

                *b = '\0';
	 
                sum += syllable_count_approximate (a);

                if (!done) {
                    ++b;
                    while (*b && isspace (*b))
                        ++b;
                }

                a = b;
            }

            g_free (cpy);
            return sum;
        }
    }

    /* Remove final silent e */
    c = cpy;
    while (*c) ++c;
    if (c != cpy && *(c-1) == 'e')
        *(c-1) = '\0';

    /* First, count vowel groups */

    for (s=cpy; *s; ++s) {
        gboolean vowel = is_vowely (*s);

        if (vowel && !prev_was_vowel)
            ++count;

        prev_was_vowel = vowel;
    }

    /* Next, add and subtract syllables as necessary */
  
    for (i=0; subsyl[i]; ++i) {
        if (baby_regexp_match (cpy, subsyl[i]))
            --count;
    }

    for (i=0; addsyl[i]; ++i) {
        if (baby_regexp_match (cpy, addsyl[i]))
            ++count;
    }

    g_free (cpy);

    /* Only allow a count of zero if the word doesn't contain
       any alphanumeric characters. */
    if (count == 0) {
        for (s = str; *s; ++s) {
            if (isalnum (*s)) {
                count = 1;
                break;
            }
        }
    }

#if 0
    {
        static GHashTable *msg = NULL;
        if (msg == NULL)
            msg = g_hash_table_new (g_str_hash, g_str_equal);
        if (!g_hash_table_lookup (msg, cpy)) {
            g_message ("Overrided: %s (%d)", str, count);
            g_hash_table_insert (msg, cpy, GINT_TO_POINTER (1));
        }
    }
#endif
       

    return count;
}

int
syllable_count_from_decomp (Phoneme *decomp)
{
    int i, count = 0;

    g_return_val_if_fail (decomp != NULL, -1);

    for (i = 0; decomp[i]; ++i)
        if (PHONEME_IS_EXPLICITLY_STRESSED (decomp[i]))
            ++count;

    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PyObject *
py_syllable_count_approximate (PyObject *self, PyObject *args)
{
    char *str;
    if (! PyArg_ParseTuple (args, "s", &str))
        return NULL;

    return Py_BuildValue ("i", syllable_count_approximate (str));
}

PyObject *
py_syllable_count_from_decomp (PyObject *self, PyObject *args)
{
    PyObject *py_decomp;
    Phoneme *decomp;
    int count;

    if (! PyArg_ParseTuple (args, "O", &py_decomp))
        return NULL;

    decomp = phoneme_decomp_from_py (py_decomp);
    if (decomp)
        count = syllable_count_from_decomp (decomp);
    else
        count = -1;
    g_free (decomp);

    return Py_BuildValue ("i", count);
}


/* $Id: syllables.c,v 1.2 2001/03/04 19:07:29 trow Exp $ */
