/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <Python.h>

#include "fate.h"
#include "phoneme.h"
#include "dictionary.h"
#include "syllable.h"
#include "rhyme.h"
#include "meter.h"
#include "token.h"
#include "text.h"
#include "tokenmodel.h"

static PyMethodDef gnoetics_methods[] = {

  /* Phoneme Methods */

  { "phoneme_code", py_phoneme_code, METH_VARARGS,
    "Get a phoneme's code value." },

  { "phoneme_code_to_string", py_phoneme_code_to_string, METH_VARARGS,
    "Get a string version of a phoneme's code value." },

  { "phoneme_stress", py_phoneme_stress, METH_VARARGS,
    "Get a phoneme's stress value." },

  { "phoneme_stress_to_string", py_phoneme_stress_to_string, METH_VARARGS,
    "Get a string version of a phoneme's stress value." },

  { "phoneme_split", py_phoneme_split, METH_VARARGS,
   "Split a phoneme into its code and stress." },

  { "phoneme_split_to_string", py_phoneme_split_to_string, METH_VARARGS,
    "Split a phoneme into string representations of its code and stress." },

  { "phoneme_is_vowel", py_phoneme_is_vowel, METH_VARARGS,
    "Is a phoneme a vowel sound?" },

  { "phoneme_is_consonant", py_phoneme_is_consonant, METH_VARARGS,
    "Is a phoneme a consonant sound?" },

  { "phoneme_is_stressed", py_phoneme_is_stressed, METH_VARARGS,
    "Is a phoneme stressed?" },

  { "phoneme_is_xstressed", py_phoneme_is_xstressed, METH_VARARGS,
    "Is a phoneme explicitly stressed?" },

  { "phoneme_to_string", py_phoneme_to_string, METH_VARARGS,
    "Convert a phoneme to a string." },

  { "phoneme_from_string", py_phoneme_from_string, METH_VARARGS,
    "Convert a string to a phoneme." },

  { "phoneme_decomp_to_string", py_phoneme_decomp_to_string, METH_VARARGS,
    "Convert a phonemic decomposition to a string." },

  { "phoneme_decomp_from_string", py_phoneme_decomp_from_string, METH_VARARGS,
    "Convert a string to a phonemic decomposition." },

  { "phoneme_equal_mod_stress", py_phoneme_equal_mod_stress, METH_VARARGS,
    "Are two phonemes equal (ignoring stresses)?" },

  { "phoneme_equal_mod_slant", py_phoneme_equal_mod_slant, METH_VARARGS,
    "Are two phonemes equal, in the sense of slant rhyme?" },

  /* Meter methods */

  { "meter_from_phoneme_decomp", py_meter_from_phoneme_decomp, METH_VARARGS,
    "Convert a phoneme decomposition into a meter string." },

  { "meter_is_valid", py_meter_is_valid, METH_VARARGS,
    "Check if a meter string is valid." },

  { "metric_match_left", py_metric_match_left, METH_VARARGS,
    "Check if two meters left-match." },

  { "metric_match_right", py_metric_match_right, METH_VARARGS,
    "Check if two meters right-match." },

  { "metric_error_left", py_metric_error_left, METH_VARARGS,
    "Rate the left-difference between two different meters." },

  { "metric_error_right", py_metric_error_right, METH_VARARGS,
    "Rate the right-difference between two different meters." },

  /* Dictionary Methods */

  { "dictionary_load", py_dictionary_load, METH_VARARGS,
    "Load dictionary data from the named file." },

  { "dictionary_lookup", py_dictionary_lookup, METH_VARARGS,
    "Look up a word's decomposition in the dictionary." },

  /* Rhyme Methods */

  { "rhyme_get_type", py_rhyme_get_type, METH_VARARGS,
    "Analyze two words or decompositions and determine how they rhyme." },

  { "rhyme_get_all", py_rhyme_get_all, METH_VARARGS,
    "Get a list of all words that rhyme with a given word or decomposition." },

  /* Syllable Methods */

  { "syllable_count_approximate", py_syllable_count_approximate, METH_VARARGS,
    "Compute a word's approximate syllable count." },

  { "syllable_count_from_decomp", py_syllable_count_from_decomp, METH_VARARGS,
    "Compute the number of syllables in a decomposition." },

  /* Token Methods */
  
  { "token_lookup",          py_token_lookup,          METH_VARARGS },
  { "token_lookup_break",    py_token_lookup_break,    METH_NOARGS },
  { "token_lookup_wildcard", py_token_lookup_wildcard, METH_NOARGS },

#if 0

  /* WordBag Methods */

  { "word_bag_new", py_word_bag_new, METH_VARARGS,
    "Create a new word bag." },

  /* Scanner Methods */

  { "scanner_new", py_scanner_new, METH_VARARGS,
    "Create a new corpus scanner." },

  { "token_get_start", py_token_get_start, METH_VARARGS,
    "Get the start token." },

  { "token_get_stop", py_token_get_stop, METH_VARARGS,
    "Get the stop token." },

  /* Text Methods */

  { "text_new", py_text_new, METH_VARARGS,
    "Create a new text object." },

  /* Markov Methods */

  { "markov_new", py_markov_new, METH_VARARGS,
    "Create a new Markov model object." },

  /* Fragment Methods */

  { "fragment_new_start", py_fragment_new_start, METH_VARARGS,
    "Create a start-token fragment." },

  { "fragment_new_stop", py_fragment_new_stop, METH_VARARGS,
    "Create a stop-token fragment." },

  { "fragment_new_break", py_fragment_new_break, METH_VARARGS,
    "Create a line-break fragment." },

  { "fragment_new_token", py_fragment_new_token, METH_VARARGS,
    "Create a fragment with a bound token." },

  { "fragment_new_language", py_fragment_new_language, METH_VARARGS,
    "Create a language fragment." },

  /* Template Methods */

  { "template_new", py_template_new, METH_VARARGS,
    "Create a new template object." },
#endif

  { NULL, NULL, 0, NULL }
};

void
initxxx_gnoetics (void)
{
  PyObject *m, *d;

  m = Py_InitModule ("xxx_gnoetics", gnoetics_methods);
  d = PyModule_GetDict (m);

  fate_seed_from_time ();

  dictionary_init ();
  py_meter_register (d);
  py_token_register (d);
  py_text_register (d);
  py_token_model_register (d);

}


