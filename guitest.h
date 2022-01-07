#include <common.h>

#if o
const IBusCIKey IBUS_CI_INIT_KEY[] = {};
#endif

const IBusCIKey IBUS_CI_TEST1_COMMIT_KEY[] = {
    { .keyval = IBUS_KEY_space, .keycode = 0, .state = 0 }
};

const IBusCITest IBUS_CI_TESTS[] = {
    {
        .preedit = { "string", "This is a test." },
        .conversion =  { NULL,  },
        .commit =  { "keys", .value = { .keys = IBUS_CI_TEST1_COMMIT_KEY } },
        .result =  { "string", "This is a test. " }
    }
};

const IBusCICases IBUS_CI_TEST_CASES = {
    .init = NULL,
    .tests = IBUS_CI_TESTS
};
