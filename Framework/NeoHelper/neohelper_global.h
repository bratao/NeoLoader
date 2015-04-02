#ifndef NEOHELPER_GLOBAL_H
#define NEOHELPER_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef NEOHELPER_LIB
# define NEOHELPER_EXPORT Q_DECL_EXPORT
#elif NEOHELPER_STATIC
# define NEOHELPER_EXPORT
#else
# define NEOHELPER_EXPORT Q_DECL_IMPORT
#endif

#endif // NEOHELPER_GLOBAL_H
