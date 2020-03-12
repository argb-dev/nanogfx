#ifdef __APPLE__
#include <stdio.h>
#define NG_METAL_SUPPORT
#define NG_DEBUG
#define NG_IMPLEMENT
#include "nanogfx.h"

static const char shader[] = "\
struct UniformData {\
   metal::float4x4 model;\
};\
struct VIn {\
   float4 position;\
   float4 color;\
};\
struct VOut {\
   float4 position [[position]];\
   float4 color;\
};\
vertex VOut vProgram(device VIn *v [[buffer(0)]], constant UniformData &udata [[buffer(1)]], uint vid [[vertex_id]]) {\
   VOut out;\
   out.position = udata.model * v[vid].position;\
   out.color = v[vid].color;\
   return out;\
}\
fragment float4 fProgram(VOut v [[stage_in]]) {\
   return v.color;\
}\
";


typedef struct {
    float model[16];
} UniformData;

typedef struct {
    float position[4];
    float color[4];
} Vertex;


#define _dbg(...) { fprintf( stderr,"=>[%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

#define _err(...) { fprintf( stderr,"[ERROR][%s][%d][%s]",__FILE__,__LINE__,__FUNCTION__); fprintf( stderr,__VA_ARGS__); fprintf( stderr,"\n");}

static nanoGFX::nGSurface app;
static id<MTLDevice> device = nil;
static id<MTLRenderPipelineState> _pipelineState;
static id<MTLBuffer> vb, ub;
static id<MTLCommandQueue> commandQueue;
static MTKView* view = NULL;
static const Vertex vData[] =
{
    { { 1, -1, 0, 1}, {1, 0, 1, 1} },
    { {-1, -1, 0, 1}, {0, 1, 0, 1} },
    { {-1,  1, 0, 1}, {0, 1, 1, 1} },
    { { 1,  1, 0, 1}, {0, 1, 1, 1} },
    { { 1, -1, 0, 1}, {1, 0, 1, 1} },
    { {-1,  1, 0, 1}, {0, 1, 1, 1} }
};

static int init(int w, int h)
{
    _dbg("init!");
    NSError *error = NULL;
    view = [(NSWindow*)app.getHandle() contentView];
    device = (id<MTLDevice>)app.getDevice();
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    NSString* program = [[NSString alloc] initWithUTF8String:shader];
    id<MTLLibrary> _library = [device newLibraryWithSource:program options:nil error:&error];
    if(!_library) {
        _err("failed to compile shaders [%s]!", [[error localizedDescription] UTF8String]);
        return -1;
    }

    pipelineDesc.vertexFunction = [_library newFunctionWithName:@"vProgram"];
    pipelineDesc.fragmentFunction = [_library newFunctionWithName:@"fProgram"];
    pipelineDesc.colorAttachments[0].pixelFormat = [view currentDrawable].texture.pixelFormat;
    _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if(!_pipelineState) {
       _err("failed to create pipeline! [%s]", [[error localizedDescription] UTF8String]);
       return -1;
    }
    vb = [device newBufferWithBytes:vData length:sizeof(vData) options:MTLResourceCPUCacheModeDefaultCache];
    ub = [device newBufferWithLength:sizeof(UniformData)  options:MTLResourceCPUCacheModeWriteCombined];
    commandQueue = [device newCommandQueue];
    return 0;
}

static void draw()
{
    static double last_time = (double)nanoGFX::nGTime::get();
    double act_time = (double)nanoGFX::nGTime::get();
    double r = (act_time - last_time)/1000 * M_PI*2/360 * 10 ;
    float cos = cosf(r);
    float sin = sinf(r);
    float mtx[16] = {cos*1.0,sin,0,0, -sin,cos*1.0,0,0, 0,0,1.0,0, 0,0,0,1};
    UniformData u;
    memcpy(u.model, mtx, sizeof(mtx));
    void* uniformTgt = [ub contents];
    memcpy(uniformTgt, &u, sizeof(UniformData));

    MTLRenderPassDescriptor* passDescriptor = [view currentRenderPassDescriptor];
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

    [commandEncoder setRenderPipelineState:_pipelineState];
    [commandEncoder setVertexBuffer:vb offset:0 atIndex:0];
    [commandEncoder setVertexBuffer:ub offset:0 atIndex:1];
    [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    [commandEncoder endEncoding];
    [commandBuffer presentDrawable:[view currentDrawable]];
    [commandBuffer commit];
}


static void eventHandler(const nanoGFX::nGSurface& surface, const nanoGFX::nGEvent& ev, void* user_data)
{
    if(ev.type == nanoGFX::nGEvent::WINDOW_REPAINT) {
       draw();
 //      [NSApp terminate:nil];
    }

    if(ev.type == nanoGFX::nGEvent::WINDOW_DESTROY || ev.type == nanoGFX::nGEvent::KEY_DOWN) {
        [NSApp terminate:nil];
    }
}

int main(int argc, char* argv[])
{
    printf("testtest\n");
    [NSAutoreleasePool new];
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    app.setEventHandler(eventHandler,NULL);
    app.create(400,400, nanoGFX::SURFACE_NONE);
    app.createMetalSurface();
    if(init(app.getWidth(), app.getHeight()) == 0) {
      [NSApp run];
    }
    return 0;
}
#else
# warning invalid platform
#endif
