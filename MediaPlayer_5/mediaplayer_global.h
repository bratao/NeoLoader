#ifndef MEDIAPLAYER_GLOBAL_H
#define MEDIAPLAYER_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef MEDIAPLAYER_LIB
# define MEDIAPLAYER_EXPORT Q_DECL_EXPORT
#else
# define MEDIAPLAYER_EXPORT Q_DECL_IMPORT
#endif

#endif // MEDIAPLAYER_GLOBAL_H
