/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "dictionary.h"

#include "syllable.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define PRONUNCIATION_DICTNAME "cmudict/cmudict.0.6"

typedef struct _RhymeTreeNode RhymeTreeNode;
struct _RhymeTreeNode {
    PhonemeCode code;
    RhymeTreeNode *branch[PHONEME_LAST];
    GSList *list;
};

static RhymeTreeNode *rhyme_tree_root = NULL;
static RhymeTreeNode *rhyme_tree_slanted_root = NULL;
static GMemChunk     *rhyme_node_chunks = NULL;

static RhymeTreeNode *
node_new (PhonemeCode code)
{
    RhymeTreeNode *node;
  
    if (rhyme_node_chunks == NULL) {
        rhyme_node_chunks = g_mem_chunk_create (RhymeTreeNode,
                                                1000,
                                                G_ALLOC_ONLY);
    }
    node = g_chunk_new0 (RhymeTreeNode, rhyme_node_chunks);
    node->code = code;
    return node;
}

static void
rhyme_index_word (DictionaryWord *dword)
{
    RhymeTreeNode *node, *snode;
    int i;
    Phoneme phon;
    PhonemeCode code, scode;

    if (dword->decomp == NULL)
        return;

    if (rhyme_tree_root == NULL)
        rhyme_tree_root = node_new (0);
    node = rhyme_tree_root;

    if (rhyme_tree_slanted_root == NULL)
        rhyme_tree_slanted_root = node_new (0);
    snode = rhyme_tree_slanted_root;

    for (i = 0; dword->decomp[i]; ++i) {
        phon = dword->decomp[i];
        code = PHONEME_TO_CODE (phon);
        scode = code;
        if (PHONEME_IS_VOWEL (code))
            scode = 0;

        if (node->branch[code] == NULL)
            node->branch[code] = node_new (code);
        node = node->branch[code];

        if (snode->branch[scode] == NULL)
            snode->branch[scode] = node_new (scode);
        snode = snode->branch[scode];

        if (PHONEME_IS_STRESSED (phon))
            break;
    }

    if (dword->decomp[i]) {
        node->list = g_slist_prepend (node->list, dword);
        snode->list = g_slist_prepend (snode->list, dword);
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GHashTable *dictionary = NULL;
static GMemChunk  *dict_chunks = NULL;
static GStaticRecMutex dict_mutex = G_STATIC_REC_MUTEX_INIT;

DictionaryWord *
dictionary_add_word (const char      *word,
                     Phoneme         *decomp)
{
    DictionaryWord *dword;
    char *query_word;

    g_return_val_if_fail (word != NULL, NULL);

    g_static_rec_mutex_lock (&dict_mutex);

    if (dictionary == NULL) {
        dictionary = g_hash_table_new (g_str_hash, g_str_equal);
    }

    query_word = g_ascii_strdown (word, -1);
    g_strstrip (query_word);
    dword = g_hash_table_lookup (dictionary, query_word);

    if (dword == NULL || (decomp != NULL && dword->decomp == NULL)) {

        gboolean add_word = FALSE;
        const char *w;

        if (dword == NULL) {

            if (dict_chunks == NULL) {
                dict_chunks = g_mem_chunk_create (DictionaryWord,
                                                  1000,
                                                  G_ALLOC_ONLY);
            }

            dword = g_chunk_new0 (DictionaryWord, dict_chunks);

            dword->word = query_word;

            /* pre-compute a hash key */
            dword->hash = 0xdeadbeef;
            for (w = word; *w; ++w)
                dword->hash = 17 * dword->hash + (long)*w;

            add_word = TRUE;
        } else {
            g_free (query_word);
        }
    
        dword->decomp = decomp;
        if (decomp)
            rhyme_index_word (dword);

        if (add_word)
            g_hash_table_insert (dictionary, dword->word, dword);
    }

    g_static_rec_mutex_unlock (&dict_mutex);

    return dword;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
remove_excess_whitespace (char *str)
{
    char *r = str, *w = str;
    gboolean last_was_space = TRUE;

    while (*r) {
        if (isspace (*r) && ! last_was_space) {
            *w = *r;
            ++w;
            last_was_space = TRUE;
        } else {
            *w = *r;
            ++w;
            last_was_space = FALSE;
        }
        ++r;
    }
    *w = '\0';
    g_strstrip (str);
}

static void
non_alpha_to_whitespace (char *str)
{
    char *p = str;
    while (*p) {
        if (! isalpha (*p))
            *p = ' ';
        ++p;
    }
    remove_excess_whitespace (str);
}

static gboolean
contains_non_alpha (const char *str)
{
    while (*str) {
        if (! isalpha (*str))
            return TRUE;
        ++str;
    }
    return FALSE;
}

static gboolean
contains_whitespace (const char *str)
{
    while (*str) {
        if (isspace (*str))
            return TRUE;
        ++str;
    }
    return FALSE;
}

DictionaryWord *
synthesize_XXXer_from_XXX (const char *normalized_word)
{
    int len = strlen (normalized_word);
    DictionaryWord *dword = NULL;

    if (len > 4 && !strcmp (normalized_word+len-2, "er")) {

        /* FIXME! */
    }

#if 0
    int len = strlen (downword);

    /* If we have XXX, synthesize XXXer */
    if (len > 4 && !strcmp (downword+len-2, "er")) {
        char *newword = g_strndup (downword, len-2);
        int i;

        dword = dictionary_get_word (newword);
        if (dword && dword->decomp) {
            Phoneme *newdecomp = g_new (Phoneme,
                                        phoneme_decomp_length (dword->decomp)+2);
            newdecomp[0] = PHONEME_JOIN (PHONEME_ER, PHONEME_NO_STRESS);
            for (i = 0; dword->decomp[i]; ++i)
                newdecomp[i+1] = dword->decomp[i];
            newdecomp[i+1] = 0;

            dword = dictionary_add_word (word, newdecomp);
        }
        g_free (newword);
    }
#endif

    return dword;
}

DictionaryWord *
synthesize_multi_word (const char *normalized_word)
{
    /* FIXME */
    return NULL;
}


DictionaryWord *
dictionary_get_word (const char *word)
{
    char *normalized_word;
    DictionaryWord *dword;

    g_return_val_if_fail (word != NULL, NULL);

    g_static_rec_mutex_lock (&dict_mutex);

    if (dictionary == NULL) {
        dictionary_load_pronunciation (NULL); /* try default */
    }

    normalized_word = g_ascii_strdown (word, -1);
    g_strstrip (normalized_word);

    dword = g_hash_table_lookup (dictionary, normalized_word);
    if (dword != NULL)
        goto finished;

    dword = synthesize_XXXer_from_XXX (normalized_word);
    if (dword != NULL)
        goto finished;

    if (contains_whitespace (normalized_word)) {
        dword = synthesize_multi_word (normalized_word);
        if (dword != NULL)
            goto finished;
    }

 finished:
    g_static_rec_mutex_unlock (&dict_mutex);
    g_free (normalized_word);
    return dword;
}

Phoneme *
dictionary_get_decomp (const char *phrase_in)
{
    char *phrase;
    char **phrasev = NULL;
    int i;
    GSList *decomps = NULL;
    Phoneme *decomp = NULL;

    if (! (phrase_in && *phrase_in))
        return NULL;

    phrase = g_strdup (phrase_in);
    remove_excess_whitespace (phrase);
    phrasev = g_strsplit (phrase, " ", -1);

    for (i = 0; phrasev[i]; ++i) {
        DictionaryWord *dword;
        Phoneme *this_decomp = NULL;

        dword = dictionary_get_word (phrasev[i]);
        if (dword != NULL) {
            this_decomp = dword->decomp;
        } else if (contains_non_alpha (phrasev[i])) {
            non_alpha_to_whitespace (phrasev[i]);
            this_decomp = dictionary_get_decomp (phrasev[i]);
        }
    
        if (this_decomp == NULL)
            goto finished;
    
        decomps = g_slist_prepend (decomps, this_decomp);
    }

    if (g_slist_length (decomps) == 1) {

        decomp = (Phoneme *) decomps->data;

    } else if (g_slist_length (decomps) > 1) {
        int len = 0, i = 0, j = 0;
        GSList *iter;

        for (iter = decomps; iter != NULL; iter = iter->next)
            len += phoneme_decomp_length (iter->data);

        decomp = g_new0 (Phoneme, len+1);
        i = 0;

        for (iter = decomps; iter != NULL; iter = iter->next) {
            Phoneme *d = iter->data;
            for (j = 0; d[j]; ++j) {
                decomp[i] = d[j];
                ++i;
            }
        }
    }

 finished:
    g_free (phrase);
    g_strfreev (phrasev);
    g_slist_free (decomps);

    return decomp;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static const char *default_dictionary_paths[] = {
#ifdef PRONUNCIATION_DICTNAME
#ifdef DICTPATH
    DICTPATH "/" PRONUNCIATION_DICTNAME,
#endif
    "dict/" PRONUNCIATION_DICTNAME,
    "../dict/" PRONUNCIATION_DICTNAME,
    "../../dict/" PRONUNCIATION_DICTNAME,
    "../../../dict/" PRONUNCIATION_DICTNAME,
#endif
    NULL
};

void
dictionary_load_pronunciation (const char *filename)
{
    FILE *in;
    char buffer[256];
    char *s, *t;
    char *word;
    Phoneme *decomp;

    /* If the filename is NULL, try the default positions */
    if (filename == NULL) {
        int i = 0;
        in = NULL;
        while (default_dictionary_paths[i] && in == NULL) {
            in = fopen (default_dictionary_paths[i], "r");
            ++i;
        }
        g_assert (in != NULL);
    } else {
        in = fopen (filename, "r");
    }

    g_return_if_fail (in != NULL);

    g_static_rec_mutex_lock (&dict_mutex);

    while (fgets (buffer, 256, in)) {

        /* Elide comments (##) */
        for (s = buffer; *s; ++s) {
            if (*s == '#' && *(s+1) == '#') {
                *s = '\0';
                break;
            }
        }

        /* Skip leading whitespace, looking for word */
        for (word = buffer; *word && isspace (*word); ++word);
    
        if (*word) { /* if line isn't empty... */
      
            /* Walk to first whitespace */
            for (t = word; *t && ! isspace (*t); ++t);
      
            if (*t) {
                /* Clobber whitespace, looking for decomp */
                for (; *t && isspace (*t); ++t) {
                    *t = '\0';
                }

                if (*t) {

                    decomp = phoneme_decomp_from_string (t);
                    dictionary_add_word (word, decomp);

                }
            }
        }

    }

    fclose(in);

    g_static_rec_mutex_unlock (&dict_mutex);
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
dictionary_foreach_by_tail (Phoneme     *decomp,
                            DictionaryFn fn,
                            gpointer     user_data)
{
    int i;
    RhymeTreeNode *node;
    Phoneme phon;
    PhonemeCode code;

    g_return_if_fail (decomp != NULL);
    g_return_if_fail (fn != NULL);

    node = rhyme_tree_slanted_root;

    for (i = 0; decomp[i] && node; ++i) {
        phon = decomp[i];
        code = PHONEME_TO_CODE (phon);
        if (PHONEME_IS_VOWEL (code))
            code = 0;
        node = node->branch[code];

        if (node && PHONEME_IS_STRESSED (phon)) {
            GSList *iter = node->list;
            while (iter != NULL) {
                fn ((DictionaryWord *) iter->data, user_data);
                iter = iter->next;
            }
            return;
        }
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gpointer
dictionary_init_thread_fn (gpointer data)
{
    dictionary_load_pronunciation (NULL); /* load default */
    return NULL;
}

void
dictionary_init (void)
{
    /* Fire off a thread that loads the dictionary. */

    if (! g_thread_supported ())
        g_thread_init (NULL);

    g_thread_create (dictionary_init_thread_fn, NULL, FALSE, NULL);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/* Python Type Magic */

typedef struct _PyDictionaryWord PyDictionaryWord;
struct _PyDictionaryWord {
    PyObject_HEAD;
    DictionaryWord *dword;
};

static PyObject *
py_dictionary_word_get_word (PyObject *self, PyObject *args)
{
    DictionaryWord *dword = dictionary_word_from_py (self);
    return Py_BuildValue ("s", dword->word);
}

static PyObject *
py_dictionary_word_get_decomp (PyObject *self, PyObject *args)
{
    DictionaryWord *dword = dictionary_word_from_py (self);
    return phoneme_decomp_to_py (dword->decomp);
}

static PyMethodDef py_dictionary_word_methods[] = {
    { "get_word", py_dictionary_word_get_word, METH_VARARGS,
      "Return the word as a string." },
    { "get_decomp", py_dictionary_word_get_decomp, METH_VARARGS,
      "Return the word's phoneme decomposition." },
    { NULL, NULL, 0, NULL }
};

static PyObject *
py_dictionary_word_getattr (PyObject *self, char *name)
{
    return Py_FindMethod (py_dictionary_word_methods, self, name);
}

static void
py_dictionary_word_dealloc (PyObject *self)
{
    PyObject_Del (self);
}

static int
py_dictionary_word_cmp (PyObject *py_a, PyObject *py_b)
{
    DictionaryWord *a = dictionary_word_from_py (py_a);
    DictionaryWord *b = dictionary_word_from_py (py_b);
  
    return strcmp (a->word, b->word);
}

static PyObject *
py_dictionary_word_repr (PyObject *self)
{
    DictionaryWord *dword = dictionary_word_from_py (self);
    char *str, *decomp_str = NULL;
    PyObject *repr;

    if (dword->decomp)
        decomp_str = phoneme_decomp_to_string (dword->decomp);

    str = g_strconcat ("<DictionaryWord '",
                       dword->word,
                       "', ",
                       decomp_str ? ", " : ">",
                       decomp_str ? decomp_str : NULL,
                       ">",
                       NULL);

    repr = Py_BuildValue ("s", str);
    g_free (decomp_str);
    g_free (str);

    return repr;
}

static long
py_dictionary_word_hash (PyObject *self)
{
    DictionaryWord *dword = dictionary_word_from_py (self);
    return dword->hash;
}

static PyTypeObject py_dictionary_word_type_info = {
    PyObject_HEAD_INIT(NULL)
    0,
    "DictionaryWord",
    sizeof(PyDictionaryWord),
    0,
    py_dictionary_word_dealloc, /*tp_dealloc*/
    NULL,                       /*tp_print*/
    py_dictionary_word_getattr, /*tp_getattr*/
    NULL,                       /*tp_setattr*/
    py_dictionary_word_cmp,     /*tp_compare*/
    py_dictionary_word_repr,    /*tp_repr*/
    NULL,                       /*tp_as_number*/
    NULL,                       /*tp_as_sequence*/
    NULL,                       /*tp_as_mapping*/
    py_dictionary_word_hash,    /*tp_hash */
};

PyObject *
dictionary_word_to_py (DictionaryWord *dword)
{
    PyDictionaryWord *py_dword;

    if (dword == NULL) {
        Py_INCREF (Py_None);
        return Py_None;
    }

    py_dword = PyObject_New (PyDictionaryWord,
                             &py_dictionary_word_type_info);
    py_dword->dword = dword;
    return (PyObject *) py_dword;
}

DictionaryWord *
dictionary_word_from_py (PyObject *obj)
{
    return ((PyDictionaryWord *) obj)->dword;
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PyObject *
py_dictionary_load (PyObject *self, PyObject *args)
{
    char *filename;

    if (! PyArg_ParseTuple (args, "s", &filename))
        return NULL;

    dictionary_load_pronunciation (filename);
  
    Py_INCREF (Py_None);
    return Py_None;
}

PyObject *
py_dictionary_lookup (PyObject *self, PyObject *args)
{
    char *word;
    DictionaryWord *dword;

    if (! PyArg_ParseTuple (args, "s", &word))
        return NULL;

    dword = dictionary_get_word (word);

    return dictionary_word_to_py (dword);
}

