**nanoGFX** is a small define driven graphics layer for x11,win,osx.

**API**

  **create(width,height,surfaceType,winAttr,parent)** - create surface with desired width, height, surface type, attributes (nGFlags - default 0) and parent window( default NULL)
   
   allowed surface types are:

   * *SURFACE_2D* - creates a 2D surface with 32bpp RGBA backbuffer accessed with getSurface() method
   *  *SURFACE_GL* - creates a surface with OpenGL context
   * *SURFACE_NONE* - creates a window without attached surface


  allowed attributes are currently:

   * *nGResize*  - allow the window to be resized
   * *nGMaximize*  - window can be maximized
   * *nGMinimize*  - window can be minimized
   * *nGNoTitleBar* - remove window titlebar
   * *nGDropShadow* - window drops shadow
   * *nGStayOnTop*  - window always on top
   * *nGFullscreen* - enable fullscreen
   * *nGMaximized* - show the window maximized
   * *nGHideInTaskBar* - hide the app in taskbar
   * *nGKeepOnBottom* - keep the window below others


  **destroy()** - destroy surface

  **activate()** - activate surface ( eg. set gl context to desired window )
  
  **update()** - render windows and process events

  **getSurface()** -return backbuffer pointer

  **getWidth()** - return surface width

  **getHeight()** - return surface height

  **setFocus()** - focus window

  **valid()** - return true if surface isn't destroyed

  **setWindowTitle(name)** - set new window title

  **setMouseCapture(state)** - enable/disable mouse capture

  **move(x,y)** -  move window to x,y

  **setIcon(data,width, height)** - set app icon

  **toClipboard(const std::string& data)** - copy data to clipboard

