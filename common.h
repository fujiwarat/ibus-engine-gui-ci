#include <ibus.h>

typedef struct _IBusCIKey IBusCIKey;
typedef struct _IBusCIKeySequence IBusCIKeySequence;
typedef struct _IBusCITest IBusCITest;
typedef struct _IBusCICases IBusCICases;

/**
 * IBusCIKey:
 *
 * The key is sent to @IBusEngineClass.process_key_event()
 */
struct _IBusCIKey {
    guint               keyval;
    guint               keycode;
    guint               state;
};

/**
 * IBusCIKeySequence:
 * @type: Can choose the type of either 'string' or 'keys' or NULL
 * @value: If type is 'string', the value must be @string which does not need
 *         modifiers. If type is 'keys', the value must be @keys and the
 *         format is { {keyval, keycode, state}, ... }. If type is NULL,
 *         the value is ignored.
 *
 * The test key sequence is used by @IBusCITest.preedit, commit, resut.
 */
struct _IBusCIKeySequence {
    char *type;
    union {
        char *string;
        const IBusCIKey *keys;
    } value;
};

/**
 * IBusCITest:
 * @preedit: Keys of whole a preedit text. Normally an ASCII string.
 * @conversion: Update a preedit or candidate popup window. Normally space or
 *              allow keys.
 * @commit: Keys to commit the preedit text. Normally a Return or space key.
 * @result: Expected output. Normally a multibyte or singlebyte string.
 *
 * This is a test case and Each @IBusCITest is called by @IBusCICases.
 */
struct _IBusCITest {
    char *desc;
    IBusCIKeySequence preedit;
    IBusCIKeySequence conversion;
    IBusCIKeySequence commit;
    IBusCIKeySequence result;
};

/**
 * IBusCICases:
 * @init: Has one array which is { {keysym, keycode, modifier} } and to be run
 *        before the main tests. E.g.
 *        Ctrl-space to enable Hiragana mode at Japanese IMEs.
 * @tests: The main test cases.
 *
 * This is a main test frame which is called by a CI engine in GUItest.c
 * The actual test entity is defined in GUItest.h
 */
struct _IBusCICases {
    const IBusCIKey *init;
    const IBusCITest *tests;
};

