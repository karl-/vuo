/**
 * @file
 * VuoWindowOpenGLInternal interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"
#include "VuoDisplayRefresh.h"
#include "VuoWindowRecorder.h"

#define NS_RETURNS_INNER_POINTER
#import <AppKit/AppKit.h>


@class VuoWindowOpenGLInternal;

/**
 * Helper for @c VuoWindowOpenGLInternal.
 */
@interface VuoWindowOpenGLView : NSOpenGLView
{
	void (*initCallback)(VuoGlContext glContext, float backingScaleFactor, void *);  ///< Initializes the OpenGL context.
	bool initCallbackCalled;	///< Has the init callback already been called?
	void (*resizeCallback)(VuoGlContext glContext, void *, unsigned int width, unsigned int height);  ///< Updates the OpenGL context when the view is resized.
	void (*drawCallback)(VuoGlContext glContext, void *);  ///< Draws onto the OpenGL context.
	void *drawContext;  ///< Argument to pass to callbacks (e.g. node instance data).

	bool callerRequestedRedraw;	///< True if an external caller (i.e., not resize or setFullScreen) requested that the GL view be redrawn.
	VuoDisplayRefresh displayRefresh;	///< Handles redrawing at the display refresh rate.  Only draws if @c callerRequestedRedraw.

	NSRect viewport;

	NSImage *circleImage;	///< The touch-circle mouse cursor.
	NSRect circleRect;		///< The bounding box of `circleImage`.
}

- (id)initWithFrame:(NSRect)frame
		 initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))_initCallback
	   resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))_resizeCallback
		 drawCallback:(void (*)(VuoGlContext glContext, void *))_drawCallback
		  drawContext:(void *)_drawContext;

- (void)enableTriggers;
- (void)disableTriggers;

- (void)setFullScreen:(BOOL)fullScreen onScreen:(NSScreen *)screen;
- (BOOL)isFullScreen;
- (void)scheduleRedraw;

@property(retain) VuoWindowOpenGLInternal *glWindow;  ///< The parent window; allows the view to access it while full-screen.
@property(retain) NSOpenGLContext *windowedGlContext;  ///< The OpenGL context from Vuo's context pool; allows the windw to access it while the view is full-screen.
@property(assign) dispatch_queue_t drawQueue;	///< Queue to ensure that multiple threads don't attempt to draw to the same window simultaneously.
@property NSRect viewport;	///< The viewport in which we're rendering (it might not match the view's dimensions), relative to the parent view.  In points (not pixels).

@end


/**
 * A graphics window for use by Vuo node classes.
 *
 * The graphics window renders onto an OpenGL context shared with Vuo nodes, so that it can render the
 * scenes created by those nodes. A scene is rendered by calling callbacks provided by the Vuo nodes.
 */
@interface VuoWindowOpenGLInternal : NSWindow <NSWindowDelegate>
{
	BOOL callbacksEnabled;  ///< True if the window can call its trigger callbacks.

	BOOL userResizedWindow;  ///< True if the user has manually resized the window.
	BOOL programmaticallyResizingWindow;  ///< True if a programmatic resize of the window is in progress.

	NSRect contentRectWhenWindowed;
	NSUInteger styleMaskWhenWindowed;

	NSRect contentRectCached;
	float backingScaleFactorCached;
	dispatch_queue_t contentRectQueue;  ///< Synchronizes access to contentRectCached.

	void (*showedWindow)(VuoWindowReference window);  ///< Callback to invoke when the window is shown, moved, and resized.
}

@property(retain) VuoWindowOpenGLView *glView;  ///< The OpenGL view inside this window.
@property BOOL depthBuffer;  ///< Was a depth buffer requested?
@property NSRect contentRectCached;  ///< The position and size of the window's content area.  In points (not pixels).
@property float backingScaleFactorCached;  ///< The number of pixels per point in the window's content area.
@property NSRect contentRectWhenWindowed;  ///< The position and size of the window's content area, prior to switching to full-screen.  In points (not pixels).
@property NSUInteger styleMaskWhenWindowed;  ///< The window's style mask, prior to switching to full-screen.
@property VuoCursor cursor;  ///< The current mouse cursor for this window.
@property(retain) NSString *titleBackup;	///< The window's title (stored since it gets cleared when switching to fullscreen mode).
@property(retain) VuoWindowRecorder *recorder;	///< nil if recording is inactive.
@property(retain) NSMenuItem *recordMenuItem;	///< The record/stop menu item.
@property(retain) NSURL *temporaryMovieURL;	///< The temporary file to which the movie is being saved.

- (id)initWithDepthBuffer:(BOOL)depthBuffer
			  initCallback:(void (*)(VuoGlContext glContext, float backingScaleFactor, void *))initCallback
			resizeCallback:(void (*)(VuoGlContext glContext, void *, unsigned int width, unsigned int height))resizeCallback
			  drawCallback:(void (*)(VuoGlContext glContext, void *))drawCallback
			   drawContext:(void *)drawContext;
- (void)enableTriggers:(void (*)(VuoWindowReference))showedWindow;
- (void)disableTriggers;
- (void)scheduleRedraw;
- (void)setProperties:(VuoList_VuoWindowProperty)properties;
- (void)executeWithWindowContext:(void(^)(VuoGlContext glContext))blockToExecute;
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh;
- (void)unlockAspectRatio;
- (void)setFullScreen:(BOOL)fullScreen onScreen:(NSScreen *)screen;
- (void)stopRecording;
@end
