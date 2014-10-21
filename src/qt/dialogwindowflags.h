#ifndef DIALOGWINDOWFLAGS_H
#define DIALOGWINDOWFLAGS_H
// Dialog Window Flags
#if QT_VERSION < 0x050000
 static const Qt::WindowFlags DIALOGWINDOWHINTS = Qt::WindowSystemMenuHint | Qt::WindowTitleHint;
#else
 static const Qt::WindowFlags DIALOGWINDOWHINTS = Qt::WindowCloseButtonHint | Qt::WindowTitleHint;
#endif
#endif