/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "text.h"

#include "pyutil.h"

static void
text_load_from_file (Text *txt, gboolean metadata_only)
{
    g_return_if_fail (txt != NULL);
    FILE *in;
    char buffer[512];
    GSList *token_slist = NULL;
    GSList *iter;
    int i;
    gboolean metadata = TRUE;

    in = fopen (txt->filename, "r");
    if (in == NULL) {
        g_warning ("Can't load %s", txt->filename);
        txt->length = 0;
        return;
    }

    /* Read tokens from file into token slist */
    while (fgets (buffer, sizeof (buffer), in)) {
        Token *tok;
        g_strstrip (buffer);

        if (metadata) {
            if (!g_ascii_strncasecmp (buffer, "author:", 7)) {
                if (txt->author == NULL) {
                    txt->author = g_strdup (buffer+7);
                    g_strstrip (txt->author);
                }
            } else if (!g_ascii_strncasecmp (buffer, "title:", 6)) {
                if (txt->title == NULL) {
                    txt->title = g_strdup (buffer+6);
                    g_strstrip (txt->title);
                }
            } else if (!g_ascii_strncasecmp (buffer, "sortauthor:", 11)) {
                if (txt->sort_author == NULL) {
                    txt->sort_author = g_ascii_strdown (buffer+11, -1);
                    g_strstrip (txt->sort_author);
                }
            } else if (!g_ascii_strncasecmp (buffer, "sorttitle:", 10)) {
                if (txt->sort_title == NULL) {
                    txt->sort_title = g_ascii_strdown (buffer+10, -1);
                    g_strstrip (txt->sort_title);
                }
            } else {
                metadata = FALSE;
            }
        }

        if (metadata_only && !metadata)
            break;

        if (! metadata) {
            tok = token_lookup (buffer);
            if (tok != NULL) 
                token_slist = g_slist_prepend (token_slist, tok);
        }
    }

    if (! metadata_only) {

        txt->length = g_slist_length (token_slist);
        txt->token_stream = g_new0 (Token *, txt->length);

        i = txt->length-1;
        for (iter = token_slist; iter != NULL; iter = iter->next) {
            txt->token_stream[i] = iter->data;
            --i;
        }
    }

    g_slist_free (token_slist);
    fclose (in);
}

Text *
text_new (const char *filename)
{
    Text *txt;

    g_return_val_if_fail (filename != NULL, NULL);

    if (! g_file_test (filename, G_FILE_TEST_EXISTS))
        return NULL;

    txt = g_new0 (Text, 1);
    REFCOUNT_INIT (txt);
    PYBIND_INIT (txt);

    txt->lock       = g_mutex_new ();
    txt->preloading = FALSE;
    txt->filename   = g_strdup (filename);
    txt->length     = -1;

    text_load_from_file (txt, TRUE);

    return txt;
}

static void
text_dealloc (Text *txt)
{
    // make sure this is the thread that owns the lock,
    // before attempting to unlock and free it
    if (g_mutex_trylock (txt->lock)) {
        g_mutex_unlock (txt->lock);
        g_mutex_free (txt ->lock);
    } else {
        g_error("Cannot free a lock owned by another thread.\n");
    }
    g_free (txt->filename);
    g_free (txt->title);
    g_free (txt->author);
    g_free (txt->sort_title);
    g_free (txt->sort_author);
    g_free (txt->token_stream);
    g_free (txt);
}

const char *
text_get_filename (Text *txt)
{
    g_return_val_if_fail (txt != NULL, NULL);
    return txt->filename;
}

const char *
text_get_title (Text *txt)
{
    g_return_val_if_fail (txt != NULL, NULL);
    return txt->title ? txt->title : "Unknown Title";
}

const char *
text_get_author (Text *txt)
{
    g_return_val_if_fail (txt != NULL, NULL);
    return txt->author ? txt->author : "Unknown Author";
}

const char *
text_get_sort_title (Text *txt)
{
    const char *title;

    g_return_val_if_fail (txt != NULL, NULL);
    if (txt->sort_title == NULL) {

        title = text_get_title (txt);
        if (! g_ascii_strncasecmp (title, "the ", 4))
            title += 4;
        else if (! g_ascii_strncasecmp (title, "a ", 2))
            title += 2;

        txt->sort_title = g_ascii_strdown (title, -1);

        g_strstrip (txt->sort_title);
    }

    return txt->sort_title;
}

const char *
text_get_sort_author (Text *txt)
{
    const char *author;
    const char *last_space;
    g_return_val_if_fail (txt != NULL, NULL);
    if (txt->sort_author == NULL) {

        author = text_get_author (txt);
        last_space = rindex (author, ' ');
        if (last_space) {
            char *tmp1 = g_strndup (author, last_space-author);
            char *tmp2 = g_strdup_printf ("%s %s", last_space+1, tmp1);
            txt->sort_author = g_ascii_strdown (tmp2, -1);
            g_free (tmp1);
            g_free (tmp2);
        } else {
            txt->sort_author = g_ascii_strdown (author, -1);
        }

        g_strstrip (txt->sort_author);
    }

    return txt->sort_author;
}

int
text_get_length (Text *txt)
{
    g_return_val_if_fail (txt != NULL, -1);

    g_mutex_lock (txt->lock);
    if (txt->length == -1)
        text_load_from_file (txt, FALSE);
    g_mutex_unlock (txt->lock);

    return txt->length;
}

Token *
text_get_token (Text *txt, int i)
{
    int N;

    g_return_val_if_fail (txt != NULL, NULL);

    N = text_get_length (txt);
    g_return_val_if_fail (i < N, NULL);

    if (i < 0)
        i += N;
    g_return_val_if_fail (i >= 0, NULL);

    return txt->token_stream[i];
}

static gpointer
text_load_fn (gpointer data)
{
    Text *txt = data;
    g_mutex_lock (txt->lock);
    if (txt->length == -1)
        text_load_from_file (txt, FALSE);
    g_mutex_unlock (txt->lock);
    text_unref (txt);
    return NULL;
}

void
text_preload (Text *txt)
{
    g_return_if_fail (txt != NULL);
    g_mutex_lock (txt->lock);
    if (txt->length == -1 && ! txt->preloading) {
        txt->preloading = TRUE;
        text_ref (txt);
        g_thread_create (text_load_fn, txt, FALSE, NULL);
    }
    g_mutex_unlock (txt->lock);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

REFCOUNT_CODE (Text, text);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PYBIND_CODE (Text, text);

static PyObject *
py_text_get_filename (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    return PyString_FromString (text_get_filename (txt));
}

static PyObject *
py_text_get_title (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    return PyString_FromString (text_get_title (txt));
}

static PyObject *
py_text_get_author (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    return PyString_FromString (text_get_author (txt));
}

static PyObject *
py_text_get_sort_title (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    return PyString_FromString (text_get_sort_title (txt));
}

static PyObject *
py_text_get_sort_author (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    return PyString_FromString (text_get_sort_author (txt));
}

static PyObject *
py_text_get_length (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    return PyInt_FromLong (text_get_length (txt));
}

static PyObject *
py_text_get_token (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    Token *token;
    int i;
    if (! PyArg_ParseTuple (args, "i", &i))
        return NULL;
    token = text_get_token (txt, i);
    return token_to_py (token);
}

static PyObject *
py_text_preload (PyObject *self, PyObject *args)
{
    Text *txt = text_from_py (self);
    text_preload (txt);
    Py_INCREF (Py_None);
    return Py_None;
}

static PyMethodDef py_text_methods[] = {
    { "get_filename",    py_text_get_filename,    METH_NOARGS },
    { "get_title",       py_text_get_title,       METH_NOARGS },
    { "get_author",      py_text_get_author,      METH_NOARGS },
    { "get_sort_title",  py_text_get_sort_title,  METH_NOARGS },
    { "get_sort_author", py_text_get_sort_author, METH_NOARGS },
    { "get_length",      py_text_get_length,      METH_NOARGS },
    { "get_token",       py_text_get_token,       METH_VARARGS },
    { "preload",         py_text_preload,         METH_NOARGS },

    { NULL, NULL, 0 }
};

static Text *
py_text_assemble (PyObject *args, PyObject *kwdict)
{
    Text *txt;
    char *filename;

    if (! PyArg_ParseTuple (args, "s", &filename))
        return NULL;

    txt = text_new (filename);
    if (txt == NULL) {
        PyErr_Format (PyExc_IOError,
                      "Can't open file '%s'", filename);
        return NULL;
    }

    return txt;
}

static PyObject *
py_text_repr (PyObject *self)
{
    Text *txt = text_from_py (self);
    if (txt->length == -1) {
        return PyString_FromFormat ("<Text: %s, '%s'>",
                                    text_get_author (txt),
                                    text_get_title (txt));
    }

    return PyString_FromFormat ("<Text: %s, '%s' (len=%d)>",
                                text_get_author (txt),
                                text_get_title (txt),
                                text_get_length (txt));
}

void
py_text_register (PyObject *dict)
{
    py_text_type_info.tp_repr    = py_text_repr;
    PYBIND_REGISTER_CODE (Text, text, dict);
}
