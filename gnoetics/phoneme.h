/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __PHONEME_H__
#define __PHONEME_H__

#include <glib.h>
#include <Python.h>

typedef enum {
    PHONEME_AA = 1,   // AA   odd     AA D
    PHONEME_AE = 2,   // AE   at      AE T
    PHONEME_AH = 3,   // AH   hut     HH AH T
    PHONEME_AO = 4,   // AO   ought   AO T
    PHONEME_AW = 5,   // AW   cow     K AW
    PHONEME_AY = 6,   // AY   hide    HH AY D
    PHONEME_EH = 7,   // EH   Ed      EH D
    PHONEME_ER = 8,   // ER   hurt    HH ER T
    PHONEME_EY = 9,   // EY   ate     EY T
    PHONEME_IH = 10,  // IH   it      IH T
    PHONEME_IY = 11,  // IY   eat     IY T
    PHONEME_OW = 12,  // OW   oat     OW T
    PHONEME_OY = 13,  // OY   toy     T OY
    PHONEME_UH = 14,  // UH   hood    HH UH D
    PHONEME_UW = 15,  // UW   two     T UW

    PHONEME_B  = 16,  // B    be      B IY
    PHONEME_CH = 17,  // CH   cheese  CH IY Z
    PHONEME_D  = 18,  // D    dee     D IY
    PHONEME_DH = 19,  // DH   thee    DH IY
    PHONEME_F  = 20,  // F    fee     F IY
    PHONEME_G  = 21,  // G    green   G R IY N
    PHONEME_HH = 22,  // HH   he      HH IY
    PHONEME_JH = 23,  // JH   gee     JH IY
    PHONEME_K  = 24,  // K    key     K IY
    PHONEME_L  = 25,  // L    lee     L IY
    PHONEME_M  = 26,  // M    me      M IY
    PHONEME_N  = 27,  // N    knee    N IY
    PHONEME_NG = 28,  // NG   ping    P IH NG
    PHONEME_P  = 29,  // P    pee     P IY
    PHONEME_R  = 30,  // R    read    R IY D
    PHONEME_S  = 31,  // S    sea     S IY
    PHONEME_SH = 32,  // SH   she     SH IY
    PHONEME_T  = 33,  // T    tea     T IY
    PHONEME_TH = 34,  // TH   theta   TH EY T AH
    PHONEME_V  = 35,  // V    vee     V IY
    PHONEME_W  = 36,  // W    we      W IY
    PHONEME_Y  = 37,  // Y    yield   Y IY L D
    PHONEME_Z  = 38,  // Z    zee     Z IY
    PHONEME_ZH = 39,  // ZH   seizure S IY ZH ER
    PHONEME_LAST = 40,
    PHONEME_INVALID = 41,
} PhonemeCode;

typedef enum {
    PHONEME_EMPTY_STRESS     = 0,  // No stress information for that phoneme
    PHONEME_NO_STRESS        = 1,  // Phoneme is flagged as unstressed
    PHONEME_PRIMARY_STRESS   = 2,
    PHONEME_SECONDARY_STRESS = 3,
} PhonemeStress;

typedef unsigned char Phoneme;

#define PHONEME_TO_CODE(p) ((p) & 0x3f)
#define PHONEME_TO_STRESS(p) (((p) & 0xc0) >> 6)
#define PHONEME_JOIN(c, s) ( ((c) & 0x3f) | (((s) & 0x03) << 6) )

#define PHONEME_IS_VOWEL(p) (PHONEME_TO_CODE (p) < PHONEME_B)
#define PHONEME_IS_CONSONANT(p) (PHONEME_TO_CODE (p) >= PHONEME_B)
#define PHONEME_IS_STRESSED(p) ((p) & 0x80)
#define PHONEME_IS_EXPLICITLY_STRESSED(p) (PHONEME_TO_STRESS (p))

#define PHONEME_EQ_MOD_STRESS(p1, p2) \
       (PHONEME_TO_CODE (p1) == PHONEME_TO_CODE (p2))

#define PHONEME_EQ_MOD_SLANT(p1, p2) \
        ((PHONEME_IS_VOWEL (p1) && PHONEME_IS_VOWEL (p2)) \
	 || PHONEME_EQ_MOD_STRESS (p1, p2))

const char   *phoneme_code_to_string     (PhonemeCode code);
PhonemeCode   phoneme_code_from_string   (const char *str, int *len);

const char   *phoneme_stress_to_string   (PhonemeStress stress);
PhonemeStress phoneme_stress_from_string (const char *str, int *len);

char         *phoneme_to_string          (Phoneme p);
Phoneme       phoneme_from_string        (const char *str, int *len);

int           phoneme_decomp_length      (Phoneme *decomp);

char         *phoneme_decomp_to_string   (Phoneme *decomp);
Phoneme      *phoneme_decomp_from_string (const char *str);

PyObject     *phoneme_decomp_to_py       (Phoneme *decomp);
Phoneme      *phoneme_decomp_from_py     (PyObject *py_decomp);


/* Python Extensions */

PyObject *py_phoneme_code               (PyObject *self, PyObject *args);
PyObject *py_phoneme_code_to_string     (PyObject *self, PyObject *args);
PyObject *py_phoneme_stress             (PyObject *self, PyObject *args);
PyObject *py_phoneme_stress_to_string   (PyObject *self, PyObject *args);
PyObject *py_phoneme_split              (PyObject *self, PyObject *args);
PyObject *py_phoneme_split_to_string    (PyObject *self, PyObject *args);
PyObject *py_phoneme_is_vowel           (PyObject *self, PyObject *args);
PyObject *py_phoneme_is_consonant       (PyObject *self, PyObject *args);
PyObject *py_phoneme_is_stressed        (PyObject *self, PyObject *args);
PyObject *py_phoneme_is_xstressed       (PyObject *self, PyObject *args);
PyObject *py_phoneme_to_string          (PyObject *self, PyObject *args);
PyObject *py_phoneme_from_string        (PyObject *self, PyObject *args);
PyObject *py_phoneme_decomp_to_string   (PyObject *self, PyObject *args);
PyObject *py_phoneme_decomp_from_string (PyObject *self, PyObject *args);
PyObject *py_phoneme_equal_mod_stress   (PyObject *self, PyObject *args);
PyObject *py_phoneme_equal_mod_slant    (PyObject *self, PyObject *args);


#endif /* __PHONEME_H__ */

