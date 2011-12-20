
#include "macdockiconhandler.h"

#include <QtGui/QMenu>
#include <QtGui/QWidget>

extern void qt_mac_set_dock_menu(QMenu*);

#undef slots
#include <Cocoa/Cocoa.h>

@interface DockIconClickEventHandler : NSObject
{
    MacDockIconHandler* dockIconHandler;
}

@end

@implementation DockIconClickEventHandler

- (id)initWithDockIconHandler:(MacDockIconHandler *)aDockIconHandler
{
    self = [super init];
    if (self) {
        dockIconHandler = aDockIconHandler;

        [[NSAppleEventManager sharedAppleEventManager]
            setEventHandler:self
                andSelector:@selector(handleDockClickEvent:withReplyEvent:)
              forEventClass:kCoreEventClass
                 andEventID:kAEReopenApplication];
    }
    return self;
}

- (void)handleDockClickEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
    Q_UNUSED(event)
    Q_UNUSED(replyEvent)

    if (dockIconHandler)
        dockIconHandler->handleDockIconClickEvent();
}

@end

MacDockIconHandler::MacDockIconHandler() : QObject()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    this->m_dockIconClickEventHandler = [[DockIconClickEventHandler alloc] initWithDockIconHandler:this];

    this->m_dummyWidget = new QWidget();
    this->m_dockMenu = new QMenu(this->m_dummyWidget);
    qt_mac_set_dock_menu(this->m_dockMenu);
    [pool release];
}

MacDockIconHandler::~MacDockIconHandler()
{
    [this->m_dockIconClickEventHandler release];
    delete this->m_dummyWidget;
}

QMenu *MacDockIconHandler::dockMenu()
{
    return this->m_dockMenu;
}

void MacDockIconHandler::setIcon(const QIcon &icon)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSImage *image;
    if (icon.isNull())
        image = [[NSImage imageNamed:@"NSApplicationIcon"] retain];
    else {
        QSize size = icon.actualSize(QSize(128, 128));
        QPixmap pixmap = icon.pixmap(size);
        CGImageRef cgImage = pixmap.toMacCGImageRef();
        image = [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
        CFRelease(cgImage);
    }

    [NSApp setApplicationIconImage:image];
    [image release];
    [pool release];
}

MacDockIconHandler *MacDockIconHandler::instance()
{
    static MacDockIconHandler *s_instance = NULL;
    if (!s_instance)
        s_instance = new MacDockIconHandler();
    return s_instance;
}

void MacDockIconHandler::handleDockIconClickEvent()
{
    emit this->dockIconClicked();
}
