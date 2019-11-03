#ifdef __APPLE__
#	import <Cocoa/Cocoa.h>
#endif
#include <stdio.h>
#define NG_3D_SUPPORT
#define NG_DEBUG
#define NG_IMPLEMENT
#include "nanogfx.h"
static nanoGFX::nGSurface app;
static bool done = false;
#define _dbg(...) { fprintf( stderr,"=>[%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

#define _err(...) { fprintf( stderr,"[ERROR][%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

static void init(int w, int h)
{
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,w,h,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glShadeModel(GL_SMOOTH);
    glClearColor(0,0.4,0.2,1.0);
    glClearDepth(1.f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

static void draw()
{
    static float rot = 0.f;

    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
#if 1
    glPushMatrix();
    glColor4f(1.f,0.7f,0.75f,0.5f);
    float cx = app.getWidth()/2;
    float cy = app.getHeight()/2;
    float s = 10;
    glTranslatef(cx,cy,0);
    glRotatef(rot,0,0,1);
    glTranslatef(-cx,-cy,0);
    rot+=1.1;
    glBegin(GL_QUADS);
    glVertex3f(cx-s,cy-s,0);
    glVertex3f(cx+s,cy-s,0);
    glVertex3f(cx+s,cy+s,0);
    glVertex3f(cx-s,cy+s,0);
    glEnd();
    glPopMatrix();
    int err = 0;
    if ((err = glGetError()) != 0) _err("glGetError(): %d", err); 
#endif
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
    app.create(400,400, nanoGFX::SURFACE_3D);
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
    return 0;
}
