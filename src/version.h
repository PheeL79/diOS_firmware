#ifndef _VERSION_H_
#define _VERSION_H_

#include "revision.h"

//-----------------------------------------------------------------------------
typedef enum {
    VER_LBL_PRE_ALPHA = 1,
    VER_LBL_ALPHA,
    VER_LBL_BETA,
    VER_LBL_RELEASE_CANDIDATE,
    VER_LBL_RELEASE_TO_MANUFACTURING,
    VER_LBL_GENERAL_AVAILABILITY,
    VER_LBL_END_OF_LIFE
} VersionLabel;

//-----------------------------------------------------------------------------
static const Version version = {
    .maj = 0,
    .min = 2,
    .bld = BUILD,
    .rev = REVISION,
    .lbl = VER_LBL_ALPHA,
};

static const char ver_lbl[8][4] =
{
    {""},
    {"pa"},
    {"a"},
    {"b"},
    {"rc"},
    {"rtm"},
    {"ga"},
    {"eol"}
};

#endif // _VERSION_H_
