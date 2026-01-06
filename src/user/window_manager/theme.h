#ifndef THEME_H
#define THEME_H

#define THEME_BASE

#ifdef THEME_MUI
#   include "mui.h"
#endif

#ifdef THEME_FLUENT
#   include "fluent.h"
#endif

#ifdef THEME_BASE
#   include "mui.h"
#endif

#endif // THEME_H