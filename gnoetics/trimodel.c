/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifdef CONFIG_H
#include <config.h>
#endif
#include "trimodel.h"

#include "fate.h"

#define CMP(x, y) (((x) > (y)) - ((x) < (y)))

typedef struct _TrimodelElement TrimodelElement;
struct _TrimodelElement {
    int key_offset;
    int syl_offset;

    Token *t1;
    Token *t2;
    Token *soln;

    Text  *text; /* source text */
    int    pos;  /* position of soln in that text */
};

/* 
   Note: We don't hold a reference to the text in the TrimodelElement
   structure, since the Trimodel itself will hold a reference in the
   text_list.
*/

static int
trimodel_element_cmp_pair (const TrimodelElement *elt,
                           const Token           *t1,
                           const Token           *t2)
{
    guint x_id, y_id;

    x_id = GPOINTER_TO_UINT (elt->t1);
    y_id = GPOINTER_TO_UINT (t1);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (elt->t2);
    y_id = GPOINTER_TO_UINT (t2);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    return 0;
}

static int
trimodel_element_cmp (gconstpointer x_ptr,
                      gconstpointer y_ptr)
{
    const TrimodelElement *x = x_ptr;
    const TrimodelElement *y = y_ptr;
    guint x_id, y_id;

    x_id = GPOINTER_TO_UINT (x->t1);
    y_id = GPOINTER_TO_UINT (y->t1);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (x->t2);
    y_id = GPOINTER_TO_UINT (y->t2);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = token_get_syllables (x->soln);
    y_id = token_get_syllables (y->soln);
    if (x_id != y_id)
        return CMP (x_id, y_id);

    x_id = GPOINTER_TO_UINT (x->soln);
    y_id = GPOINTER_TO_UINT (y->soln);
    return CMP (x_id, y_id);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct _RhymeTreeNode RhymeTreeNode;
struct _RhymeTreeNode {
    PhonemeCode code;
    RhymeTreeNode *branch[PHONEME_LAST];
    GSList *list;
};

static RhymeTreeNode *
node_new (PhonemeCode code, GMemChunk *chunk)
{
    RhymeTreeNode *node;
    node = g_chunk_new0 (RhymeTreeNode, chunk);
    node->code = code;
    return node;
}

static void
node_index (RhymeTreeNode **root_ptr,
            RhymeTreeNode **sroot_ptr,
            Token          *token,
            GMemChunk      *chunk,
            GAllocator     *allocator)
{
    Phoneme *decomp;
    RhymeTreeNode *node, *snode;
    int i;
    Phoneme phon;
    PhonemeCode code, scode;
    
    g_assert (root_ptr != NULL);
    g_assert (sroot_ptr != NULL);
    g_assert (token != NULL);
    g_assert (chunk != NULL);
    g_assert (allocator != NULL);

    decomp = token_get_decomp (token);
    if (decomp == NULL)
        return;

    if (*root_ptr == NULL)
        *root_ptr = node_new (0, chunk);
    node = *root_ptr;

    if (*sroot_ptr == NULL)
        *sroot_ptr = node_new (0, chunk);
    snode = *sroot_ptr;

    for (i = 0; decomp[i]; ++i) {
        phon = decomp[i];
        code = PHONEME_TO_CODE (phon);
        scode = PHONEME_IS_VOWEL (code) ? 0 : code;

        if (node->branch[code] == NULL)
            node->branch[code] = node_new (code, chunk);
        node = node->branch[code];

        if (snode->branch[scode] == NULL)
            snode->branch[scode] = node_new (scode, chunk);
        snode = snode->branch[scode];

        if (PHONEME_IS_STRESSED (phon))
            break;
    }

    if (decomp[i]) {
        g_slist_push_allocator (allocator);
        node->list  = g_slist_prepend (node->list,  token);
        snode->list = g_slist_prepend (snode->list, token);
        g_slist_pop_allocator ();
    }
}

GSList *
node_walk (RhymeTreeNode *node,
           gboolean       is_slanted,
           Token         *tok)
{
    Phoneme *decomp;
    Phoneme phon;
    PhonemeCode code;
    int i;

    decomp = token_get_decomp (tok);
    if (decomp == NULL)
        return NULL;

    for (i = 0; decomp[i] && node; ++i) {
        phon = decomp[i];
        code = PHONEME_TO_CODE (phon);
        if (is_slanted && PHONEME_IS_VOWEL (code))
            code = 0;
        
        node = node->branch[code];

        if (node && PHONEME_IS_STRESSED (phon))
            return node->list;
    }

    return NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct _RhymeInfo RhymeInfo;
struct _RhymeInfo {
    Token *tok;
    int count_slant;
    int count_false;
    int count_true;

    double p_slant;
    double p_false;
    double p_true;
};

static RhymeInfo *
rhyme_info_new (Token     *tok,
                GMemChunk *chunk)
{
    RhymeInfo *info;
    info = g_chunk_new0 (RhymeInfo, chunk);
    info->tok = tok;
    info->count_slant = 0;
    info->count_false = 0;
    info->count_true  = 0;
    return info;
}

static void
rhyme_info_incr (RhymeInfo *info,
                 RhymeType  rt)
{
    switch (rt) {
        /* We aren't breaking on purpose. */
        case RHYME_TRUE:
            ++info->count_true;
        case RHYME_FALSE:
            ++info->count_false;
        case RHYME_SLANT:
            ++info->count_slant;
            break;
        default:
            /* do nothing */
            break;
    }
}

static int
rhyme_info_get (RhymeInfo *info,
                RhymeType  rt)
{
    switch (rt) {
        case RHYME_SLANT:
            return info->count_slant;
        case RHYME_FALSE:
            return info->count_false;
        case RHYME_TRUE:
            return info->count_true;
        default:
            return 0;
    }
}

static void
rhyme_info_set_p (RhymeInfo *info,
                  RhymeType  rt,
                  double     p)
{
    switch (rt) {
        case RHYME_SLANT:
            info->p_slant = p;
            break;
        case RHYME_FALSE:
            info->p_false = p;
            break;
        case RHYME_TRUE:
            info->p_true = p;
            break;
        default:
            g_assert_not_reached ();
            break;
    }
}

static int
rhyme_info_slant_cmp (RhymeInfo **a,
                      RhymeInfo **b)
{
    return (*b)->count_slant - (*a)->count_slant;
}

static int
rhyme_info_false_cmp (RhymeInfo **a,
                      RhymeInfo **b)
{
    return (*b)->count_false - (*a)->count_false;
}

static int
rhyme_info_true_cmp (RhymeInfo **a,
                     RhymeInfo **b)
{
    return (*b)->count_true - (*a)->count_true;
}

static GCompareFunc
rhyme_info_get_cmp_fn (RhymeType type)
{
    switch (type) {
        case RHYME_SLANT:
            return (GCompareFunc) rhyme_info_slant_cmp;
        case RHYME_FALSE:
            return (GCompareFunc) rhyme_info_false_cmp;
        case RHYME_TRUE:
            return (GCompareFunc) rhyme_info_true_cmp;
        default:
            g_assert_not_reached ();
            return NULL;
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

Trimodel *
trimodel_new (void)
{
    Trimodel *tri;

    tri = g_new0 (Trimodel, 1);
    REFCOUNT_INIT (tri);
    PYBIND_INIT (tri);

    tri->lock = g_mutex_new ();

    tri->array_AB_C = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));
    tri->array_AC_B = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));
    tri->array_BC_A = g_array_new (FALSE, FALSE, sizeof (TrimodelElement));

    tri->rhyme_info_chunks = g_mem_chunk_create (RhymeInfo,
                                                 1000, G_ALLOC_ONLY);
    tri->rhyme_info_table = g_hash_table_new (NULL, NULL);

    tri->is_prepped = FALSE;

    return tri;
}

static void
trimodel_dealloc (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    g_array_free (tri->array_AB_C, TRUE);
    g_array_free (tri->array_AC_B, TRUE);
    g_array_free (tri->array_BC_A, TRUE);

    if (tri->all_tokens != NULL)
        g_ptr_array_free (tri->all_tokens, TRUE);

    if (tri->is_leading != NULL)
        g_hash_table_destroy (tri->is_leading);
    if (tri->is_trailing != NULL)
        g_hash_table_destroy (tri->is_trailing);

    g_mem_chunk_destroy (tri->rhyme_info_chunks);
    g_hash_table_destroy (tri->rhyme_info_table);
    
    g_list_foreach (tri->text_list, (GFunc) text_unref, NULL);
    g_list_free (tri->text_list);
    
    g_free (tri);
}

static void
trimodel_sort_single_array (Trimodel *tri,
                            GArray   *array)
{
    int i, first_i, firstsyl_i, syl_n;
    TrimodelElement *elt, *first, *firstsyl;

    if (array->len < 2)
        return;

    g_array_sort (array, trimodel_element_cmp);

    /* Attach offset information. */
    first_i = firstsyl_i = 0;
    first = firstsyl = &g_array_index (array, TrimodelElement, 0);
    syl_n = -1;

    for (i = 1; i < array->len; ++i) {
        elt = &g_array_index (array, TrimodelElement, i);

        if (first->t1 == elt->t1 && first->t2 == elt->t2) {
            first->key_offset = i - first_i;
            elt->key_offset = first_i - i;

            if (syl_n == token_get_syllables (elt->soln)) {
                firstsyl->syl_offset = i - firstsyl_i;
                elt->syl_offset = firstsyl_i - i;
            } else {
                firstsyl_i = i;
                firstsyl = elt;
                firstsyl->syl_offset = 0;
                syl_n = token_get_syllables (firstsyl->soln);
            }

        } else {
            first_i = firstsyl_i = i;
            first = firstsyl = elt;
            first->key_offset = 0;
            firstsyl->syl_offset = 0;
        }
    }
}

static void
trimodel_add_triple (Trimodel *tri,
                     Text     *text,
                     int       pos_of_a,
                     Token    *a,
                     Token    *b,
                     Token    *c)
{
    TrimodelElement elt;

    g_return_if_fail (tri != NULL);
    g_return_if_fail (a != NULL);
    g_return_if_fail (b != NULL);
    g_return_if_fail (c != NULL);

    elt.key_offset = 0;
    elt.syl_offset = 0;
    
    elt.t1 = a;
    elt.t2 = b;
    elt.soln = c;
    elt.text = text;
    elt.pos  = pos_of_a + 2;
    g_array_append_val (tri->array_AB_C, elt);

    elt.t1 = a;
    elt.t2 = c;
    elt.soln = b;
    elt.text = text;
    elt.pos  = pos_of_a + 1;
    g_array_append_val (tri->array_AC_B, elt);

    elt.t1 = b;
    elt.t2 = c;
    elt.soln = a;
    elt.text = text;
    elt.pos = pos_of_a;
    g_array_append_val (tri->array_BC_A, elt);

    tri->is_prepped = FALSE;
}

void
trimodel_add_text (Trimodel *tri,
                   Text     *txt)
{
    g_return_if_fail (tri != NULL);
    g_return_if_fail (txt != NULL);

    g_assert (! tri->is_prepped);
    tri->text_list = g_list_append (tri->text_list, text_ref (txt));
}

static void
trimodel_study_texts (Trimodel *tri)
{
    GList *iter;
    Token *brk, *tok, *window[3];
    int i, j, N;

    g_return_if_fail (tri != NULL);

    brk = token_lookup_break ();
    g_assert (brk != NULL);

    for (iter = tri->text_list; iter != NULL; iter = iter->next) {
        Text *txt = iter->data;

        window[0] = brk;
        window[1] = brk;
        window[2] = brk;
    
        N = text_get_length (txt);
        for (i = 0; i < N; ++i) {
            tok = text_get_token (txt, i);
            
            j = 0;
            do {
                window[0] = window[1];
                window[1] = window[2];
                window[2] = tok;
                trimodel_add_triple (tri, txt, i-2+j, 
                                     window[0], window[1], window[2]);
                ++j;
            } while (j < 2 && token_is_break (tok));
        }
    }
}

static void
trimodel_build_all_tokens_array (Trimodel *tri)
{
    GHashTable *uniq;
    TrimodelElement *elt;
    int i;

    g_return_if_fail (tri != NULL);

    if (tri->all_tokens != NULL)
        g_ptr_array_free (tri->all_tokens, TRUE);

    tri->all_tokens = g_ptr_array_new ();

    uniq = g_hash_table_new (NULL, NULL);

    elt = & g_array_index (tri->array_AB_C, TrimodelElement, 0);

    for (i = 0; i < tri->array_AB_C->len; ++i) {

        elt = & g_array_index (tri->array_AB_C, TrimodelElement, i);

        if (i == 0 && g_hash_table_lookup (uniq, elt->t1) == NULL) {
            g_ptr_array_add (tri->all_tokens, elt->t1);
            g_hash_table_insert (uniq, elt->t1, elt->t1);
        }

        if (i <= 1 && g_hash_table_lookup (uniq, elt->t2) == NULL) {
            g_ptr_array_add (tri->all_tokens, elt->t2);
            g_hash_table_insert (uniq, elt->t2, elt->t2);
        }
        
        if (g_hash_table_lookup (uniq, elt->soln) == NULL) {
            g_ptr_array_add (tri->all_tokens, elt->soln);
            g_hash_table_insert (uniq, elt->soln, elt->soln);
        }
    }
}

static void
trimodel_build_leading_trailing (Trimodel *tri)
{
    GHashTable *not_head, *not_tail;
    TrimodelElement *elt;
    int i;

    g_return_if_fail (tri != NULL);

    if (tri->is_leading != NULL)
        g_hash_table_destroy (tri->is_leading);

    if (tri->is_trailing != NULL)
        g_hash_table_destroy (tri->is_trailing);

    tri->is_leading = g_hash_table_new (NULL, NULL);
    tri->is_trailing = g_hash_table_new (NULL, NULL);

    not_head = g_hash_table_new (NULL, NULL);
    not_tail = g_hash_table_new (NULL, NULL);

    for (i = 0; i < tri->array_AB_C->len; ++i) {
        elt = &g_array_index (tri->array_AB_C, TrimodelElement, i);

        if (!token_is_break (elt->t1) && !token_is_break (elt->t2)) {
            if (g_hash_table_lookup (not_tail, elt->t1) == NULL)
                g_hash_table_insert (not_tail, elt->t1, elt->t1);
            if (g_hash_table_lookup (not_head, elt->t2) == NULL)
                g_hash_table_insert (not_head, elt->t2, elt->t2);
        }

        if (!token_is_break (elt->t2) && !token_is_break (elt->soln)) {
            if (g_hash_table_lookup (not_tail, elt->t2) == NULL)
                g_hash_table_insert (not_tail, elt->t2, elt->t2);
            if (g_hash_table_lookup (not_head, elt->soln) == NULL)
                g_hash_table_insert (not_head, elt->soln, elt->soln);
        }
    }

    for (i = 0; i < tri->all_tokens->len; ++i) {
        Token *t = g_ptr_array_index (tri->all_tokens, i);

        if (token_is_break (t))
            continue;
        
        if (g_hash_table_lookup (not_head, t) == NULL) 
            g_hash_table_insert (tri->is_leading, t, t);

        if (g_hash_table_lookup (not_tail, t) == NULL)
            g_hash_table_insert (tri->is_trailing, t, t);

    }

    g_hash_table_destroy (not_head);
    g_hash_table_destroy (not_tail);
}

static void
trimodel_build_rhyme_table (Trimodel *tri)
{
    RhymeTreeNode *root_rhyme_tree = NULL;
    RhymeTreeNode *root_slanted_rhyme_tree = NULL;

    GPtrArray *all_rhyme_info;

    GMemChunk *rhyme_tree_chunks;
    GAllocator *rhyme_tree_allocator;

    int i, j, k;
    RhymeType rt;

    
    /* Step 1: Build Rhyme tree for fast lookup of tokens by
       the tails of their phoneme decompositions. */
    
    rhyme_tree_chunks = g_mem_chunk_create (RhymeTreeNode, 1000, G_ALLOC_ONLY);
    rhyme_tree_allocator = g_allocator_new ("rhyme allocator", 1000);

    for (i = 0; i < tri->all_tokens->len; ++i) {
        Token *tok = g_ptr_array_index (tri->all_tokens, i);
        node_index (&root_rhyme_tree, &root_slanted_rhyme_tree,
                    tok, rhyme_tree_chunks, rhyme_tree_allocator);
    }

    /* Step 2: Compute the rhyme counts for each token. */

    all_rhyme_info = g_ptr_array_sized_new (tri->all_tokens->len);

    for (i = 0; i < tri->all_tokens->len; ++i) {
        Token *tok = g_ptr_array_index (tri->all_tokens, i);
        GSList *iter;
        RhymeInfo *info;

        info = rhyme_info_new (tok, tri->rhyme_info_chunks);
        g_hash_table_insert (tri->rhyme_info_table, tok, info);
        g_ptr_array_add (all_rhyme_info, info);

        if (token_is_break (tok) || token_is_punctuation (tok))
            continue;

        iter = node_walk (root_slanted_rhyme_tree, TRUE, tok);
        while (iter != NULL) {
            Token *other = iter->data;

            if (other != tok) {
                rt = rhyme_get_type (token_get_decomp (other),
                                     token_get_decomp (tok));
                rhyme_info_incr (info, rt);
            }
            
            iter = iter->next;
        }
    }

    /* Step 3: Compute p-values */
    
    for (rt = RHYME_SLANT; rt <= RHYME_TRUE; ++rt) {
        GCompareFunc sort_fn = NULL;

        sort_fn = rhyme_info_get_cmp_fn (rt);
        g_ptr_array_sort (all_rhyme_info, sort_fn);

        g_assert (tri->all_tokens->len == all_rhyme_info->len);
        i = 0;
        while (i < all_rhyme_info->len) {
            RhymeInfo *info_i = g_ptr_array_index (all_rhyme_info, i);
            int count_i = rhyme_info_get (info_i, rt);
            double p;

            for (j = i+1; j < all_rhyme_info->len; ++j) {
                RhymeInfo *info_j = g_ptr_array_index (all_rhyme_info, j);
                int count_j = rhyme_info_get (info_j, rt);

                if (count_i != count_j)
                    break;
            }

            p = ((i + (i + j - 1))/2.0) / (double) tri->all_tokens->len;

            for (k = i; k < j; ++k) {
                RhymeInfo *info_k = g_ptr_array_index (all_rhyme_info, k);
                rhyme_info_set_p (info_k, rt, p);
            }

            i = j;
        }
    }
    
    /* cleanup: */
    /* Free all of the rhyme tree memory */
    g_mem_chunk_destroy (rhyme_tree_chunks);
    g_allocator_free (rhyme_tree_allocator);
    g_ptr_array_free (all_rhyme_info, TRUE);
}

static gpointer
trimodel_prepare_fn (gpointer data)
{
    Trimodel *tri = data;

    g_mutex_lock (tri->lock);

    trimodel_study_texts (tri);

    trimodel_sort_single_array (tri, tri->array_AB_C);
    trimodel_sort_single_array (tri, tri->array_AC_B);
    trimodel_sort_single_array (tri, tri->array_BC_A);

    trimodel_build_all_tokens_array (tri);

    trimodel_build_leading_trailing (tri);

    trimodel_build_rhyme_table (tri);

    tri->is_prepped = TRUE;

    g_mutex_unlock (tri->lock);

    trimodel_unref (tri);
    return NULL;
}

void
trimodel_prepare (Trimodel *tri)
{
    g_return_if_fail (tri != NULL);

    if (! tri->is_prepped) {
        g_mutex_lock (tri->lock);
        if (! tri->is_prepped) {
            trimodel_ref (tri);
            g_thread_create (trimodel_prepare_fn, tri, FALSE, NULL);
        }
        g_mutex_unlock (tri->lock);
    }
}

gboolean
trimodel_is_ready (Trimodel *tri)
{
    g_return_val_if_fail (tri != NULL, FALSE);
    return tri->is_prepped;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
trimodel_query_array (Trimodel *tri,
                      GArray   *array,
                      Token    *t1,
                      Token    *t2,
                      int       min_syllables,
                      int       max_syllables,
                      int      *out_i0,
                      int      *out_i1)
{
    int a, b, i, i0, i1, cmp;
    const TrimodelElement *elt;

    g_return_val_if_fail (tri != NULL, FALSE);
    g_return_val_if_fail (t1 != NULL, FALSE);
    g_return_val_if_fail (t2 != NULL, FALSE);

    if (min_syllables >= 0 
        && max_syllables >= 0 
        && min_syllables > max_syllables)
        return FALSE;

    if (! tri->is_prepped)
        trimodel_prepare (tri);

    /* First, find a possible hit. */

    a = 0;
    b = array->len-1;
    i = -1;

    while (b-a > 1) {
        i = (a+b)/2;

        elt = &g_array_index (array, TrimodelElement, i);
        cmp = trimodel_element_cmp_pair (elt, t1, t2);

        if (cmp == 0)
            break;
        else if (cmp < 0)
            a = i;
        else /* cmp > 0 */
            b = i;

        i = -1;
    }

    if (b-a <= 1 && i != -1) {
        elt = &g_array_index (array, TrimodelElement, a);
        cmp = trimodel_element_cmp_pair (elt, t1, t2);
        if (cmp == 0)
            i = a;
        else if (a != b) {
            elt = &g_array_index (array, TrimodelElement, b);
            cmp = trimodel_element_cmp_pair (elt, t1, t2);
            if (cmp == 0)
                i = b;
        }
    }

    if (i == -1)
        return FALSE;


    /* Next, use the hit to find the beginning and end of our
       solution range. */
    g_assert (0 <= i && i < array->len);
    elt = &g_array_index (array, TrimodelElement, i);
    g_assert (elt->t1 == t1);
    g_assert (elt->t2 == t2);

    i0 = i;
    if (elt->key_offset < 0) /* move back to start */
        i0 += elt->key_offset;

    g_assert (0 <= i && i < array->len);
    elt = &g_array_index (array, TrimodelElement, i0);
    g_assert (elt->key_offset >= 0);
    g_assert (elt->t1 == t1);
    g_assert (elt->t2 == t2);

    i1 = i0 + elt->key_offset;

    /* Adjust bounds to find solutions w/ right numbers of syllables. */
    if (min_syllables >= 0) {
        while (i0 <= i1) {
            elt = &g_array_index (array, TrimodelElement, i0);
            if (min_syllables <= token_get_syllables (elt->soln)) {
                /* If min_syllables == max_syllables, we have
                   enough information to skip the separate max_syllables
                   check. */
                if (min_syllables == max_syllables) {
                    i1 = i0 + elt->syl_offset;
                    max_syllables = -1;
                }
                break;
            }
            i0 += elt->syl_offset+1;
        }
    }

    if (max_syllables >= 0) {
        while (i0 <= i1) {
            elt = &g_array_index (array, TrimodelElement, i0);
            if (token_get_syllables (elt->soln) <= max_syllables)
                break;
            i1 -= elt->syl_offset+1;
        }
    }

    if (i0 > i1)
        return FALSE;

    if (out_i0)
        *out_i0 = i0;

    if (out_i1)
        *out_i1 = i1;
    
    return TRUE;
}

static gboolean
leading_test_cb (Token   *t,
                 gpointer user_data)
{
    Trimodel *tri = user_data;
    return trimodel_token_is_leading (tri, t);
}

static gboolean
trailing_test_cb (Token   *t,
                  gpointer user_data)
{
    Trimodel *tri = user_data;
    return trimodel_token_is_trailing (tri, t);
}

static double
rhyme_p_value_cb (Token *t,
                  RhymeType threshold,
                  gpointer user_data)
{
    Trimodel *tri = user_data;
    return trimodel_rhyme_p_value (tri, t, threshold);
}

gint
trimodel_query (Trimodel       *tri,
                Token          *token_a,
                Token          *token_b,
                Token          *token_c,
                TokenFilter    *filter,
                Ranker         *ranker)
{

    int i0, i1, i, count;
    int wild_count;
    Token *t1 = NULL, *t2 = NULL;
    GArray *array = NULL;
    const TrimodelElement *elt;
    Token *prev_soln;
    FilterResults results;

    g_return_val_if_fail (tri != NULL, -1);
    g_return_val_if_fail (token_a != NULL, -1);
    g_return_val_if_fail (token_b != NULL, -1);
    g_return_val_if_fail (token_c != NULL, -1);

    wild_count = 0;
    if (token_is_wildcard (token_a))
        ++wild_count;
    if (token_is_wildcard (token_b))
        ++wild_count;
    if (token_is_wildcard (token_c))
        ++wild_count;
    if (wild_count != 1) {
        g_warning ("ignoring ill-formed query w/ %d wildcards", wild_count);
        return -1;
    }

    if (token_is_wildcard (token_a)) {
        t1 = token_b;
        t2 = token_c;
        array = tri->array_BC_A;
    } else if (token_is_wildcard (token_b)) {
        t1 = token_a;
        t2 = token_c;
        array = tri->array_AC_B;
    } else if (token_is_wildcard (token_c)) {
        t1 = token_a;
        t2 = token_b;
        array = tri->array_AB_C;
    } else {
        g_assert_not_reached ();
    }

    g_assert (t1 != NULL);
    g_assert (t2 != NULL);
    g_assert (array != NULL);

    if (! trimodel_query_array (tri,
                                array,
                                t1, t2,
                                filter ? filter->min_syllables : -1,
                                filter ? filter->max_syllables : -1,
                                &i0, &i1)) {
        return 0;
    }

    g_assert (i0 <= i1);

    prev_soln = NULL;
    results = FILTER_RESULTS_REJECT;
    count = 0;
    for (i = i0; i <= i1; ++i) {
        elt = &g_array_index (array, TrimodelElement, i);
        if (filter == NULL) {
            results = FILTER_RESULTS_ACCEPT;
        } else if (elt->soln == prev_soln) {
            /* retain previous value of results */
        } else {
            results = token_filter_test (filter,
                                         elt->soln,
                                         leading_test_cb,
                                         trailing_test_cb,
                                         rhyme_p_value_cb,
                                         tri);


            prev_soln = elt->soln;
        }


        if (results != FILTER_RESULTS_REJECT) {
            if (ranker != NULL) {
                ranker_add_solution (ranker,
                                     elt->soln,
                                     elt->text,
                                     elt->pos,
                                     results);
            }
            ++count;
        }
    }

    return count;
}

gint
trimodel_rhyme_count (Trimodel *tri,
                      Token    *tok,
                      RhymeType rhyme_type_threshold)
{
    RhymeInfo *info;

    g_return_val_if_fail (tri != NULL, -1);
    g_return_val_if_fail (tok != NULL, -1);

    info = g_hash_table_lookup (tri->rhyme_info_table, tok);
    if (info == NULL)
        return -1;

    switch (rhyme_type_threshold) {
        
        case RHYME_SLANT:
            return info->count_slant;
            
        case RHYME_FALSE:
            return info->count_false;

        case RHYME_TRUE:
            return info->count_true;

        default:
            g_assert_not_reached();
            break;
    }

    g_assert_not_reached ();
    return -1;
}

double
trimodel_rhyme_p_value (Trimodel *tri,
                        Token    *tok,
                        RhymeType rhyme_type_threshold)
{
    RhymeInfo *info;

    g_return_val_if_fail (tri != NULL, -1);
    g_return_val_if_fail (tok != NULL, -1);

    info = g_hash_table_lookup (tri->rhyme_info_table, tok);
    if (info == NULL)
        return -1;

    switch (rhyme_type_threshold) {
        
        case RHYME_SLANT:
            return info->p_slant;
            
        case RHYME_FALSE:
            return info->p_false;

        case RHYME_TRUE:
            return info->p_true;

        default:
            g_assert_not_reached();
            break;
    }

    g_assert_not_reached ();
    return -1;
    
}

gboolean
trimodel_token_is_leading (Trimodel *tri,
                           Token    *tok)
{
    g_return_val_if_fail (tri != NULL, FALSE);
    g_return_val_if_fail (tok != NULL, FALSE);

    return g_hash_table_lookup (tri->is_leading, tok) != NULL;
}

gboolean
trimodel_token_is_trailing (Trimodel *tri,
                            Token    *tok)
{
    g_return_val_if_fail (tri != NULL, FALSE);
    g_return_val_if_fail (tok != NULL, FALSE);

    return g_hash_table_lookup (tri->is_trailing, tok) != NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

REFCOUNT_CODE (Trimodel, trimodel);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

PYBIND_CODE (Trimodel, trimodel);

static PyObject *
py_trimodel_add_text (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    PyObject *py_txt;
    Text *txt;
    
    if (! (PyArg_ParseTuple (args, "O", &py_txt) && py_text_check (py_txt)))
        return NULL;

    txt = text_from_py (py_txt);
    trimodel_add_text (tri, txt);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_trimodel_get_texts (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    GList *iter;
    PyObject *py_list;

    py_list = PyList_New (0);

    for (iter = tri->text_list; iter != NULL; iter = iter->next) {
        Text *txt = iter->data;
        PyObject *py_txt = text_to_py (txt);
        PyList_Append (py_list, py_txt);
        Py_DECREF (py_txt);
    }

    return py_list;
}

static PyObject *
py_trimodel_prepare (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    trimodel_prepare (tri);

    Py_INCREF (Py_None);
    return Py_None;
}

static PyObject *
py_trimodel_is_ready (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    return PyBool_FromLong (trimodel_is_ready (tri));
}

static PyObject *
py_trimodel_query (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);

    PyObject *py_t1, *py_t2, *py_t3, *py_filter, *py_ranker;

    Token *t1, *t2, *t3;
    TokenFilter filter;
    Ranker *ranker;

    int count;

    if (! PyArg_ParseTuple (args, "OOOOO",
                            &py_t1, &py_t2, &py_t3,
                            &py_filter,
                            &py_ranker))
        return NULL;

    if (! (py_token_check (py_t1)
           && py_token_check (py_t2)
           && py_token_check (py_t3)
           && PyDict_Check (py_filter)
           && py_ranker_check (py_ranker))) {
        PyErr_SetString (PyExc_ValueError, "Badly-formed query");
        return NULL;
    }

    t1 = token_from_py (py_t1);
    t2 = token_from_py (py_t2);
    t3 = token_from_py (py_t3);

    token_filter_init_from_py_dict (&filter, py_filter);

    ranker = ranker_from_py (py_ranker);

    count = trimodel_query (tri, t1, t2, t3, &filter, ranker);

    return PyInt_FromLong (count);
}

static PyObject *
py_trimodel_rhyme_count (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    PyObject *py_tok;
    Token *tok;
    int rhyme_level;

    if (! PyArg_ParseTuple (args, "Oi", &py_tok, &rhyme_level))
        return NULL;

    if (! py_token_check (py_tok)) {
        PyErr_SetString (PyExc_ValueError, "Expecting a token as first argument");
        return NULL;
    }
    
    tok = token_from_py (py_tok);
    
    return PyInt_FromLong (trimodel_rhyme_count (tri, tok,
                                                 (RhymeType) rhyme_level));
}

static PyObject *
py_trimodel_rhyme_p_value (PyObject *self, PyObject *args)
{
    Trimodel *tri = trimodel_from_py (self);
    PyObject *py_tok;
    Token *tok;
    int rhyme_level;

    if (! PyArg_ParseTuple (args, "Oi", &py_tok, &rhyme_level))
        return NULL;

    if (! py_token_check (py_tok)) {
        PyErr_SetString (PyExc_ValueError, "Expecting a token as first argument");
        return NULL;
    }
    
    tok = token_from_py (py_tok);
    
    return PyFloat_FromDouble (trimodel_rhyme_p_value (tri, tok,
                                                       (RhymeType) rhyme_level));
}

static PyMethodDef py_trimodel_methods[] = {
    { "add_text",      py_trimodel_add_text,      METH_VARARGS },
    { "get_texts",     py_trimodel_get_texts,     METH_NOARGS  },
    { "prepare",       py_trimodel_prepare,       METH_NOARGS  },
    { "is_ready",      py_trimodel_is_ready,      METH_NOARGS  },
    { "query",         py_trimodel_query,         METH_VARARGS },
    { "rhyme_count",   py_trimodel_rhyme_count,   METH_VARARGS  },
    { "rhyme_p_value", py_trimodel_rhyme_p_value, METH_VARARGS  },

    { NULL, NULL, 0 }

};

static Trimodel *
py_trimodel_assemble (PyObject *args, PyObject *kwdict)
{
    return trimodel_new ();
}

void
py_trimodel_register (PyObject *dict)
{
    PYBIND_REGISTER_CODE (Trimodel, trimodel, dict);
}
