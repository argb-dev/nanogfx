#ifdef __APPLE__
#	import <Cocoa/Cocoa.h>
#endif
#include <stdio.h>
#include <stdint.h>
#define NG_DEBUG
#define NG_IMPLEMENT
#define NG_2D_SUPPORT
#include "nanogfx.h"
static nanoGFX::nGSurface app;
static bool done = false;
static int result = -1;
#define _dbg(...) { fprintf( stderr,"=>[%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

#define _err(...) { fprintf( stderr,"[ERROR][%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

static void init(int w, int h) {
}

static void draw()
{
  static uint8_t t = 0;
  ++t;
  size_t offs = 0;
  unsigned char* ptr = app.getSurface();
  for(size_t i = 0; i < app.getHeight(); ++i) {
    for(size_t j  = 0; j < app.getWidth(); ++j) {
      unsigned char c = i^(j + t);
      ptr[offs++] = c;
      ptr[offs++] = c;
      ptr[offs++] = c;
      ptr[offs++] = 0xff;
    }
  }
  result = 0;
  done = true;
}


#ifdef __APPLE__
@interface TimerTask: NSObject
@end
@implementation TimerTask
-(void)timerEvent:(NSTimer*)t {
    draw();
    app.update();
}
@end
#endif

static void eventHandler(const nanoGFX::nGSurface& surface, const nanoGFX::nGEvent& ev, void* user_data)
{
    if(ev.type == nanoGFX::nGEvent::WINDOW_CREATE) init(app.getWidth(), app.getHeight());
    if(ev.type == nanoGFX::nGEvent::WINDOW_REPAINT) draw();

    if(ev.type == nanoGFX::nGEvent::WINDOW_DESTROY || ev.type == nanoGFX::nGEvent::KEY_DOWN)
    {
#ifdef __APPLE__
        [NSApp terminate:nil];
#else
        done = true;
#endif
    }
}

int main(int argc, char* argv[])
{
    printf("testtest\n");
#ifdef __APPLE__
    [NSAutoreleasePool new];
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    printf("1\n");
#endif
    app.setEventHandler(eventHandler,NULL);
    app.create(400,400, nanoGFX::SURFACE_2D);
#if 0
    TimerTask* timerTask = [[TimerTask alloc] init];
    NSTimer* timer = [NSTimer timerWithTimeInterval:0.016f target:timerTask selector:@selector(timerEvent:) userInfo:nil repeats:YES];
    [timer autorelease];
    [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
#endif
#ifdef __APPLE__
    [NSApp run];
#else
    while(!done) {
        app.update();
        nanoGFX::nGTime::sleep(16);
    }
#endif
    return result;
}
