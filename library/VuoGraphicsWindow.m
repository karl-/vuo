/**
 * @file
 * VuoGraphicsWindow implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "module.h"
#import "VuoApp.h"
#import "VuoGraphicsWindow.h"
#import "VuoGraphicsWindowDelegate.h"
#import "VuoGraphicsWindowDrag.h"
#import "VuoGraphicsView.h"
#import "VuoScreenCommon.h"
#import <Carbon/Carbon.h>

#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title" : "VuoGraphicsWindow",
	"dependencies" : [
		"VuoApp",
		"VuoGraphicsView",
		"VuoGraphicsWindowDelegate",
		"VuoScreenCommon",
		"VuoWindowProperty",
		"VuoWindowRecorder",
		"VuoWindowReference",
		"VuoList_VuoWindowProperty"
	]
});
#endif


/// Serialize making windows fullscreen, since Mac OS X beeps at you (!) if you try to fullscreen two windows too quickly.
dispatch_semaphore_t VuoGraphicsWindow_fullScreenTransitionSemaphore;

/**
 * Initialize VuoGraphicsWindow_fullScreenTransitionSemaphore.
 */
static void __attribute__((constructor)) VuoGraphicsWindow_init()
{
	 VuoGraphicsWindow_fullScreenTransitionSemaphore = dispatch_semaphore_create(1);
}


/**
 * Private VuoGraphicsWindow data.
 */
@interface VuoGraphicsWindow ()
@property(atomic) bool callbacksEnabled;       ///< True if the window can call its trigger callbacks.
@property(retain) NSMenuItem *recordMenuItem;  ///< The record/stop menu item.
@end

@implementation VuoGraphicsWindow

/**
 * Creates a window with the default size and position.
 */
- (instancetype)initWithInitCallback:(VuoGraphicsWindowInitCallback)initCallback
			   updateBackingCallback:(VuoGraphicsWindowUpdateBackingCallback)updateBackingCallback
					  resizeCallback:(VuoGraphicsWindowResizeCallback)resizeCallback
						drawCallback:(VuoGraphicsWindowDrawCallback)drawCallback
							userData:(void *)userData
{
	NSRect mainScreenFrame = [[NSScreen mainScreen] frame];
	_contentRectWhenWindowed = NSMakeRect(mainScreenFrame.origin.x, mainScreenFrame.origin.y, 1024, 768);
	_styleMaskWhenWindowed = NSTitledWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	if (self = [super initWithContentRect:_contentRectWhenWindowed
								styleMask:_styleMaskWhenWindowed
								  backing:NSBackingStoreBuffered
									defer:NO])
	{
		self.delegate = [[VuoGraphicsWindowDelegate alloc] initWithWindow:self];
		self.releasedWhenClosed = NO;

		_cursor = VuoCursor_Pointer;

		_callbacksEnabled = NO;

		self.contentMinSize = NSMakeSize(16,16);

		self.acceptsMouseMovedEvents = YES;

		self.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;

		[self initDrag];
		[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];

		_userResizedWindow = NO;
		_programmaticallyResizingWindow = NO;

		char *title = VuoApp_getName();
		self.title = [NSString stringWithUTF8String:title];
		free(title);

		_backingScaleFactorCached = self.backingScaleFactor;

		VuoGraphicsView *gv = [[VuoGraphicsView alloc] initWithInitCallback:initCallback
													   updateBackingCallback:updateBackingCallback
														  backingScaleFactor:_backingScaleFactorCached
															  resizeCallback:resizeCallback
																drawCallback:drawCallback
																	userData:userData];
		self.contentView = gv;
		[gv release];

		_contentRectCached = gv.frame;

		// Remove the 2 grey pixels from bottom left/right corners.
		self.backgroundColor = [NSColor clearColor];
	}

	return self;
}

/**
 * Schedules the OpenGL view to be redrawn. This can be used in both windowed and full-screen mode.
 *
 * @threadAny
 */
- (void)scheduleRedraw
{
	VuoGraphicsView *gv = self.contentView;
	gv.needsIOSurfaceRedraw = true;
}

/**
 * Allow this window to become main, even when it's borderless.
 */
- (BOOL)canBecomeMainWindow
{
	return YES;
}

/**
 * Allow this window to become key, even when it's borderless.
 */
- (BOOL)canBecomeKeyWindow
{
	return YES;
}

/**
 * Updates the menu bar with this window's menus (View > Full Screen).
 *
 * @threadMain
 */
- (void)becomeMainWindow
{
	[super becomeMainWindow];

	if (!_isInMacFullScreenMode)
		[self setFullScreenPresentation:self.isFullScreen];

	NSMenu *fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
	_recordMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:@selector(toggleRecording) keyEquivalent:@"e"];
	[_recordMenuItem setKeyEquivalentModifierMask:NSCommandKeyMask|NSAlternateKeyMask];
	[fileMenu addItem:_recordMenuItem];
	NSMenuItem *fileMenuItem = [[NSMenuItem new] autorelease];
	[fileMenuItem setSubmenu:fileMenu];

	NSMenu *viewMenu = [[[NSMenu alloc] initWithTitle:@"View"] autorelease];
	NSMenuItem *fullScreenMenuItem = [[[NSMenuItem alloc] initWithTitle:@"Full Screen" action:@selector(toggleFullScreen) keyEquivalent:@"f"] autorelease];
	[viewMenu addItem:fullScreenMenuItem];
	NSMenuItem *viewMenuItem = [[NSMenuItem new] autorelease];
	[viewMenuItem setSubmenu:viewMenu];

	NSMutableArray *windowMenuItems = [NSMutableArray arrayWithCapacity:3];
	[windowMenuItems addObject:fileMenuItem];
	[windowMenuItems addObject:viewMenuItem];
	self.oldMenu = (NSMenu *)VuoApp_setMenuItems(windowMenuItems);

	[self updateUI];
}

/**
 * Updates the menu bar with the host app's menu prior to when this window was activated.
 */
- (void)resignMainWindow
{
	[super resignMainWindow];

	// No need to change presentation when resigning, since:
	//    - other VuoGraphicsWindows change presentation on becomeMainWindow
	//    - Mac OS X changes presentation when another app becomes active
	//
	// Also, if two VuoGraphicsWindows are fullscreen on separate displays,
	// switching out of fullscreen presentation will cause the menubar and dock to flicker
	// when clicking from one display to another.
//	if (!_isInMacFullScreenMode)
//		[self setFullScreenPresentation:NO];

	VuoApp_setMenu(_oldMenu);
	self.oldMenu = nil;
}

/**
 * This method is implemented to avoid the system beep for keyboard events, which may be handled by vuo.keyboard nodes.
 */
- (void)keyDown:(NSEvent *)event
{
	if ([event keyCode] == kVK_Escape && [self isFullScreen])
		[super keyDown:event];
}

/**
 * Updates various UI elements to match current internal state.
 */
- (void)updateUI
{
	if (_recorder)
		_recordMenuItem.title = @"Stop Recording…";
	else
		_recordMenuItem.title = @"Start Recording";
}

/**
 * Sets up the window to call trigger functions.
 *
 * @threadAny
 */
- (void)enableShowedWindowTrigger:(VuoGraphicsWindowShowedWindowCallback)showedWindow requestedFrameTrigger:(VuoGraphicsWindowRequestedFrameCallback)requestedFrame
{
	_callbacksEnabled = YES;

	_showedWindow = showedWindow;
	_showedWindow(VuoWindowReference_make(self));

	_requestedFrame = requestedFrame;

	VuoGraphicsView *gv = self.contentView;
	[gv enableTriggers];
}

/**
 * Stops the window from calling trigger functions.
 *
 * @threadAny
 */
- (void)disableTriggers
{
	VuoGraphicsView *gv = self.contentView;
	[gv disableTriggers];

	_callbacksEnabled = NO;

	_showedWindow = NULL;
	_requestedFrame = NULL;
}

/**
 * Returns YES if this window is currently fullscreen.
 */
- (bool)isFullScreen
{
	return _isInMacFullScreenMode || self.styleMask == 0;
}


/**
 * Store the title, so we can re-apply it after returning from fullscreen mode.
 */
- (void)setTitle:(NSString *)title
{
	self.titleBackup = title;
	super.title = title;
}

/**
 * Applies a list of properties to this window.
 *
 * @threadMain
 */
- (void)setProperties:(VuoList_VuoWindowProperty)properties
{
	unsigned int propertyCount = VuoListGetCount_VuoWindowProperty(properties);
	for (unsigned int i = 1; i <= propertyCount; ++i)
	{
		VuoWindowProperty property = VuoListGetValue_VuoWindowProperty(properties, i);
//		VLog("%s",VuoWindowProperty_getSummary(property));

		if (property.type == VuoWindowProperty_Title)
			self.title = property.title ? [NSString stringWithUTF8String:property.title] : @"";
		else if (property.type == VuoWindowProperty_FullScreen)
		{
			NSScreen *requestedScreen = VuoScreen_getNSScreen(property.screen);

			// If we're already fullscreen, and the property tells us to switch to a different screen,
			// temporarily switch back to windowed mode, so that we can cleanly switch to fullscreen on the new screen.
			if ([self isFullScreen] && property.fullScreen)
			{
				NSInteger requestedDeviceId = [[[requestedScreen deviceDescription] objectForKey:@"NSScreenNumber"] integerValue];
				NSInteger currentDeviceId = [[[self.screen deviceDescription] objectForKey:@"NSScreenNumber"] integerValue];
				if (requestedDeviceId != currentDeviceId)
					[self setFullScreen:NO onScreen:nil];
			}

			[self setFullScreen:property.fullScreen onScreen:requestedScreen];
		}
		else if (property.type == VuoWindowProperty_Position)
		{
			NSRect propertyInPoints = NSMakeRect(property.left, property.top, 0, 0);
			if (property.unit == VuoCoordinateUnit_Pixels)
				propertyInPoints = [self.contentView convertRectFromBacking:propertyInPoints];

			NSRect mainScreenRect = [[[NSScreen screens] objectAtIndex:0] frame];
			if ([self isFullScreen])
				_contentRectWhenWindowed.origin = NSMakePoint(propertyInPoints.origin.x, mainScreenRect.size.height - _contentRectWhenWindowed.size.height - propertyInPoints.origin.y);
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];
				self.frameOrigin = NSMakePoint(propertyInPoints.origin.x, mainScreenRect.size.height - contentRect.size.height - propertyInPoints.origin.y);
			}
		}
		else if (property.type == VuoWindowProperty_Size)
		{
			NSRect propertyInPoints = NSMakeRect(0, 0, property.width, property.height);
			if (property.unit == VuoCoordinateUnit_Pixels)
				propertyInPoints = [self.contentView convertRectFromBacking:propertyInPoints];
			_maintainsPixelSizeWhenBackingChanges = (property.unit == VuoCoordinateUnit_Pixels);

			if ([self isFullScreen])
				_contentRectWhenWindowed.size = propertyInPoints.size;
			else
			{
				NSRect contentRect = [self contentRectForFrameRect:[self frame]];

				// Adjust the y position by the change in height, so that the window appears to be anchored in its top-left corner
				// (instead of its bottom-left corner as the system does by default).
				contentRect.origin.y += contentRect.size.height - propertyInPoints.size.height;

				contentRect.size = NSMakeSize(propertyInPoints.size.width, propertyInPoints.size.height);
				@try
				{
					[self setFrame:[self frameRectForContentRect:contentRect] display:YES animate:NO];
				}
				@catch (NSException *e)
				{
					char *description = VuoLog_copyCFDescription(e);
					VUserLog("Error: Couldn't change window size to %lldx%lld: %s", property.width, property.height, description);
					free(description);
				}
			}
		}
		else if (property.type == VuoWindowProperty_AspectRatio)
		{
			if (property.aspectRatio < 1./10000
			 || property.aspectRatio > 10000)
			{
				VUserLog("Error: Couldn't change window aspect ratio to %g since it's unrealistically narrow.", property.aspectRatio);
				continue;
			}

			NSRect contentRect = [self contentRectForFrameRect:[self frame]];
			[self setAspectRatioToWidth:contentRect.size.width height:contentRect.size.width/property.aspectRatio];
		}
		else if (property.type == VuoWindowProperty_AspectRatioReset)
			[self unlockAspectRatio];
		else if (property.type == VuoWindowProperty_Resizable)
		{
			[[self standardWindowButton:NSWindowZoomButton] setEnabled:property.resizable];
			if ([self isFullScreen])
				_styleMaskWhenWindowed = property.resizable ? (_styleMaskWhenWindowed | NSResizableWindowMask) : (_styleMaskWhenWindowed & ~NSResizableWindowMask);
			else
				self.styleMask =         property.resizable ? ([self styleMask]       | NSResizableWindowMask) : ([self styleMask]       & ~NSResizableWindowMask);
		}
		else if (property.type == VuoWindowProperty_Cursor)
		{
			_cursor = property.cursor;
			[self invalidateCursorRectsForView:self.contentView];
		}
	}
}

/**
 * -[NSWindow frame] isn't updated continually during a drag.
 * Apparently the old Carbon API is the only way to get its current size/position.
 *
 * Via Jonathan 'Wolf' Rentzsch http://rentzsch.com .
 */
- (NSRect)liveFrame
{
	Rect qdRect;
	extern OSStatus GetWindowBounds(WindowRef window, WindowRegionCode regionCode, Rect *globalBounds);

	GetWindowBounds([self windowRef], kWindowStructureRgn, &qdRect);

	return NSMakeRect(qdRect.left,
					  (float)CGDisplayPixelsHigh(kCGDirectMainDisplay) - qdRect.bottom,
					  qdRect.right - qdRect.left,
					  qdRect.bottom - qdRect.top);
}

/**
 * Constrains the window's aspect ratio to @c pixelsWide/pixelsHigh. Also resizes the
 * window to @c pixelsWide by @c pixelsHigh, unless the user has manually resized the
 * window (in which case the width is preserved) or the requested size is larger than the
 * window's screen (in which case the window is scaled to fit the screen).
 *
 * @threadMain
 */
- (void)setAspectRatioToWidth:(unsigned int)pixelsWide height:(unsigned int)pixelsHigh
{
	NSSize newAspect = NSMakeSize(pixelsWide, pixelsHigh);
	if (NSEqualSizes([self contentAspectRatio], newAspect))
		return;

	// Sets the constraint when the user resizes the window (but doesn't affect the window's current size).
	self.contentAspectRatio = newAspect;

	if ([self isFullScreen])
	{
		if (_showedWindow)
			_showedWindow(VuoWindowReference_make(self));
		return;
	}

	CGFloat desiredWidth = pixelsWide;
	CGFloat desiredHeight = pixelsHigh;
	CGRect windowFrame = [self liveFrame];
	CGFloat aspectRatio = (CGFloat)pixelsWide / (CGFloat)pixelsHigh;

	// Adjust the width and height if the user manually resized the window.
	if (_userResizedWindow)
	{
		// Preserve the width, scale the height.
		desiredWidth = CGRectGetWidth(windowFrame);
		desiredHeight = CGRectGetWidth(windowFrame) / aspectRatio;
	}

	// Adjust the width and height if they don't fit the screen.
	NSRect screenFrame = [[self screen] visibleFrame];
	NSRect maxContentRect = [self contentRectForFrameRect:screenFrame];
	if (desiredWidth > maxContentRect.size.width || desiredHeight > maxContentRect.size.height)
	{
		CGFloat maxContentAspectRatio = maxContentRect.size.width / maxContentRect.size.height;
		if (aspectRatio >= maxContentAspectRatio)
		{
			// Too wide, so scale it to the maximum horizontal screen space.
			desiredWidth = maxContentRect.size.width;
			desiredHeight = maxContentRect.size.width / aspectRatio;
		}
		else
		{
			// Too tall, so scale it to the maximum vertical screen space.
			desiredWidth = maxContentRect.size.height * aspectRatio;
			desiredHeight = maxContentRect.size.height;
		}
	}

	NSSize newContentSize = NSMakeSize(desiredWidth, desiredHeight);
	NSSize newWindowSize = [self frameRectForContentRect:NSMakeRect(0, 0, newContentSize.width, newContentSize.height)].size;

	// Preserve the window's top left corner position. (If you just resize, it preserves the bottom left corner position.)
	CGFloat topY = CGRectGetMinY(windowFrame) + CGRectGetHeight(windowFrame);
	NSRect newWindowFrame = NSMakeRect(CGRectGetMinX(windowFrame), topY - newWindowSize.height, newWindowSize.width, newWindowSize.height);

	_programmaticallyResizingWindow = YES;
	[self setFrame:newWindowFrame display:YES];
	_programmaticallyResizingWindow = NO;

	if (_showedWindow)
		_showedWindow(VuoWindowReference_make(self));
}

/**
 * Removes the aspect ratio constraint set by @ref VuoWindowOpenGl_setAspectRatio.
 *
 * @threadMain
 */
- (void)unlockAspectRatio
{
	self.resizeIncrements = NSMakeSize(1,1);
}

/**
 * Enables/disables Vuo's custom fullscreen behavior (@see -setFullScreen:onScreen:).
 */
- (void)setFullScreenPresentation:(bool)enabled
{
	if (enabled)
	{
		// Don't raise the window level, since the window still covers the screen even when it's not focused.
//		self.level = NSScreenSaverWindowLevel;

		// Instead, just hide the dock and menu bar.
		((NSApplication *)NSApp).presentationOptions = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
	}
	else
	{
//		self.level = NSNormalWindowLevel;
		((NSApplication *)NSApp).presentationOptions = NSApplicationPresentationDefault;
	}
}

/**
 * Switches between full-screen and windowed mode.
 *
 * "Fullscreen" can mean 3 different things:
 *
 * 1. `-[NSView enterFullScreenMode:withOptions:]`
 *    - 10.5+
 *    - we don't use this at all
 * 2. Vuo's custom fullscreen mode —
 *    - instant (no transition animation)
 *    - achieved by hiding the titlebar, menubar, and dock, and resizing the window
 *    - can go fullscreen on any user-specified VuoScreen
 *    - can have multiple windows fullscreen simultaneously (regardless of screensHaveSeparateSpaces setting)
 *    - if `screensHaveSeparateSpaces=NO`, menu bar reappears when focusing a non-fullscreen window (bad when doing a performance)
 *    - should not change the window level, since that prevents app switching and Mission Control from working properly
 * 3. `-[NSWindow toggleFullScreen:]`
 *    - a.k.a. Mac-fullscreen mode
 *    - a.k.a. the arrows in the top-right of the window on 10.9 and prior
 *    - a.k.a. the arrows inside the green button in the top-left of the window on 10.10 and later
 *    - optional in 10.7 – 10.10, mandatory in 10.11+
 *    - creates a space for every fullscreen window (some users consider this a feature)
 *    - painfully slow transition animation
 *    - if `screensHaveSeparateSpaces=NO`, can only have one window fullscreen (other screen is blank)
 *
 * `screensHaveSeparateSpaces` is `System Preferences > Mission Control > Displays have separate Spaces`.
 *
 * Vuo uses #3 if it allows simultaneous fullscreen windows on separate displays
 * (i.e., when both running on 10.9+ and screensHaveSeparateSpaces=YES),
 * and otherwise uses #2 (which allows simultaneous fullscreen windows on separate displays regardless of OS version and settings).
 *
 * https://b33p.net/kosada/node/8729
 * https://b33p.net/kosada/node/8730
 * https://b33p.net/kosada/node/11899
 *
 * @threadMain
 */
- (void)setFullScreen:(BOOL)wantsFullScreen onScreen:(NSScreen *)screen
{
	_programmaticallyResizingWindow = true;

	if (wantsFullScreen && ![self isFullScreen])
	{
		// Switch into fullscreen mode.


		// Save the position, size, and style, to be restored when switching back to windowed mode.
		_contentRectWhenWindowed = [self contentRectForFrameRect:self.frame];
		_styleMaskWhenWindowed = [self styleMask];

		// Move the window to the specified screen.
		// (Necessary even for Mac-fullscreen mode, since it fullscreens on whichever screen the window is currently on.)
		if (screen && ![[self screen] isEqualTo:screen])
		{
			NSRect windowFrame = self.frame;
			NSRect screenFrame = screen.frame;
			self.frameOrigin = NSMakePoint(screenFrame.origin.x + screenFrame.size.width/2  - windowFrame.size.width/2,
										   screenFrame.origin.y + screenFrame.size.height/2 - windowFrame.size.height/2);
		}


		// Use Mac-fullscreen mode by default, since it's compatible with Mission Control,
		// and since (unlike Kiosk Mode) the menu bar stays hidden on the fullscreen display
		// when you focus a window on a different display (such as when livecoding during a performance).
		bool useMacFullScreenMode = true;

		SInt32 macMinorVersion;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		Gestalt(gestaltSystemVersionMinor, &macMinorVersion);
#pragma clang diagnostic pop

		// On Mac OS 10.8 and prior, only a single window could go Mac-fullscreen at once, so don't use Mac-fullscreen mode on those versions.
		if (macMinorVersion <= 8)
			useMacFullScreenMode = false;

		// On Mac OS 10.9 and later, if `System Preferences > Mission Control > Displays have separate Spaces` is unchecked,
		// only a single window could go Mac-fullscreen at once, so don't use Mac-fullscreen mode in that case.
		else
		{
			SEL screensHaveSeparateSpacesSel = @selector(screensHaveSeparateSpaces);
			IMP screensHaveSeparateSpaces = [NSScreen methodForSelector:screensHaveSeparateSpacesSel];
			if (!screensHaveSeparateSpaces([NSScreen class], screensHaveSeparateSpacesSel))
				useMacFullScreenMode = false;
		}


		if (useMacFullScreenMode)
		{
			// If a size-locked window enters Mac-fullscreen mode,
			// its height increases upon exiting Mac-fullscreen mode.
			// Mark it resizeable while fullscreen, to keep its size from changing (!).
			self.styleMask |= NSResizableWindowMask;

			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				dispatch_semaphore_wait(VuoGraphicsWindow_fullScreenTransitionSemaphore, DISPATCH_TIME_FOREVER);
				dispatch_async(dispatch_get_main_queue(), ^{
					_programmaticallyTransitioningFullScreen = true;
					[self toggleFullScreen:nil];
				});
			});
		}
		else
		{
			[self setFullScreenPresentation:YES];

			NSSize car = self.contentAspectRatio;
			self.styleMask = 0;
			if (!NSEqualSizes(car, NSMakeSize(0,0)))
				self.contentAspectRatio = car;

			// Make the window take up the whole screen.
			[self setFrame:self.screen.frame display:YES];

			[self finishFullScreenTransition];
		}
	}
	else if (!wantsFullScreen && [self isFullScreen])
	{
		// Switch out of fullscreen mode.

		if (_isInMacFullScreenMode)
		{
			dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
				dispatch_semaphore_wait(VuoGraphicsWindow_fullScreenTransitionSemaphore, DISPATCH_TIME_FOREVER);
				dispatch_async(dispatch_get_main_queue(), ^{
					_programmaticallyTransitioningFullScreen = true;
					[self toggleFullScreen:nil];
					self.styleMask = _styleMaskWhenWindowed;
				});
			});
		}
		else
		{
			[self setFullScreenPresentation:NO];

			NSSize car = self.contentAspectRatio;
			self.styleMask = _styleMaskWhenWindowed;
			self.title = _titleBackup;
			if (!NSEqualSizes(car, NSMakeSize(0,0)))
				self.contentAspectRatio = car;

			[self setFrame:[self frameRectForContentRect:_contentRectWhenWindowed] display:YES];

			[self finishFullScreenTransition];
		}
	}
}

/**
 * Ends the programmatic-resizing event-block, and explicitly fires the resize handler once.
 *
 * To be called when the delegate receives `-windowDidEnter/ExitFullScreen:`.
 */
- (void)finishFullScreenTransition
{
	_programmaticallyResizingWindow = false;

	[self.delegate windowDidResize:nil];
//	((VuoGraphicsView *)self.contentView).needsIOSurfaceRedraw = true;

	[self makeFirstResponder:self];
}

/**
 * Switches between full-screen and windowed mode.
 *
 * @threadMain
 */
- (void)toggleFullScreen
{
	[self setFullScreen:![self isFullScreen] onScreen:nil];
}

/**
 * Starts or stops recording a movie of the currently-selected window.
 *
 * @threadMain
 */
- (void)toggleRecording
{
	if (!self.recorder)
	{
		// Start recording to a temporary file (rather than first asking for the filename, to make it quicker to start recording).
		self.temporaryMovieURL = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:[[NSProcessInfo processInfo] globallyUniqueString]]];
		_recorder = [[VuoWindowRecorder alloc] initWithWindow:self url:_temporaryMovieURL];

		[self updateUI];
	}
	else
		[self stopRecording];
}

/**
 * Ask the user for a permanent location to move the temporary movie file to.
 */
- (void)promptToSaveMovie
{
	NSSavePanel *sp = [NSSavePanel savePanel];
	[sp setTitle:@"Save Movie"];
	[sp setNameFieldLabel:@"Save Movie To:"];
	[sp setPrompt:@"Save"];
	[sp setAllowedFileTypes:@[@"mov"]];

	char *title = VuoApp_getName();
	sp.nameFieldStringValue = [NSString stringWithUTF8String:title];
	free(title);

	if ([sp runModal] == NSFileHandlingPanelCancelButton)
		goto done;

	NSError *error;
	if (![[NSFileManager defaultManager] moveItemAtURL:_temporaryMovieURL toURL:[sp URL] error:&error])
	{
		if ([error code] == NSFileWriteFileExistsError)
		{
			// File exists.  Since, in the NSSavePanel, the user said to Replace, try replacing it.
			if (![[NSFileManager defaultManager] replaceItemAtURL:[sp URL]
					  withItemAtURL:_temporaryMovieURL
					 backupItemName:nil
							options:0
				   resultingItemURL:nil
							  error:&error])
			{
				// Replacement failed; show error…
				NSAlert *alert = [NSAlert alertWithError:error];
				[alert runModal];

				// …and give the user another chance.
				[self promptToSaveMovie];
			}
			goto done;
		}

		NSAlert *alert = [NSAlert alertWithError:error];
		[alert runModal];
	}

done:
	self.temporaryMovieURL = nil;
}

/**
 * Stops recording a movie of the currently-selected window.
 *
 * Does nothing if recording is not currently enabled.
 *
 * @threadMain
 */
- (void)stopRecording
{
	if (!_recorder)
		return;

	[_recorder finish];
	[_recorder release];
	_recorder = nil;

	[self updateUI];

	[self setFullScreen:NO onScreen:nil];

	[self promptToSaveMovie];
}

/**
 * Makes Escape and Command-. exit full-screen mode.
 *
 * @threadMain
 */
- (void)cancelOperation:(id)sender
{
	[self setFullScreen:NO onScreen:nil];
}

/**
 * Executes the specifed block on the window's rootContext, then returns.
 * Ensures that nobody else is using the OpenGL context at that time
 * (by synchronizing with the window's @c drawQueue).
 *
 * @threadAny
 */
- (void)executeWithWindowContext:(void(^)(VuoGlContext glContext))blockToExecute
{
	VuoGraphicsView *gv = self.contentView;
	CGLContextObj rootContext = gv.rootContext;
	CGLLockContext(rootContext);
	CGLSetCurrentContext(rootContext);

	blockToExecute(rootContext);

	CGLSetCurrentContext(NULL);
	CGLUnlockContext(rootContext);
}

/**
 * Called when the window is being closed (and thus rendering should stop).
 */
- (void)close
{
	VuoGraphicsView *gv = self.contentView;
	[gv close];

	[super close];
}

/**
 * Releases instance variables.
 */
- (void)dealloc
{
	// https://b33p.net/kosada/node/12123
	// Cocoa leaks the contentView unless we force it to resign firstResponder status.
	[self makeFirstResponder:nil];

	[super dealloc];
}

@end