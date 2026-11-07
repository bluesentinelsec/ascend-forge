/**
 * @file android_library.c
 * @brief Translation-unit anchor for the Android Prefab shared library.
 *
 * The packaged symbols come from the whole-archived core library. This file
 * keeps that shared library a C target.
 */

#include "ascend_forge/version.h"

void AndroidLibrary_Anchor(void)
{
    (void)Version_String();
}
