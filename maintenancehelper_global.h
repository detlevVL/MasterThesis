#ifndef MAINTENANCEHELPER_GLOBAL_H
#define MAINTENANCEHELPER_GLOBAL_H

#include <QtGlobal>

#if defined(MAINTENANCEHELPER_LIBRARY)
#  define MAINTENANCEHELPERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define MAINTENANCEHELPERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // MAINTENANCEHELPER_GLOBAL_H

