/**
 * @file result.c
 * @brief Success and failure results that borrow their text.
 */

#include "ascend_forge/result.h"

static bool text_usable(Utf8 text)
{
    return text.data != NULL;
}

Result Result_Ok(void)
{
    Result result;

    result.ok = true;
    result.code = 0u;
    result.expected = Utf8_Empty();
    result.actual = Utf8_Empty();
    result.next = Utf8_Empty();
    return result;
}

bool Result_Fail(Uint32 code, Utf8 expected, Utf8 actual, Utf8 next, Result *out)
{
    if (out == NULL) {
        SDL_Log("Result_Fail expected an out parameter, received NULL");
        return false;
    }
    if (code == 0u) {
        SDL_Log("Result_Fail expected a non-zero code, received 0");
        return false;
    }
    if (!text_usable(expected) || !text_usable(actual) || !text_usable(next)) {
        SDL_Log("Result_Fail expected text with a real pointer");
        return false;
    }

    out->ok = false;
    out->code = code;
    out->expected = expected;
    out->actual = actual;
    out->next = next;
    return true;
}
