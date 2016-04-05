/**
 * @file
 * VuoWindow interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"
#include "VuoGlContext.h"

#include <stdint.h>

void VuoApp_init(void);
void VuoApp_fini(void);
bool VuoApp_isMainThread(void);

/**
 * A window containing a text edit widget.
 */
typedef void * VuoWindowText;

/**
 * A window containing an OpenGL view.
 */
typedef void * VuoWindowOpenGl;

VuoWindowText VuoWindowText_make(void);
void VuoWindowText_enableTriggers
(
		VuoWindowText w,
		VuoOutputTrigger(typedLine, VuoText),
		VuoOutputTrigger(typedWord, VuoText),
		VuoOutputTrigger(typedCharacter, VuoText)
);
void VuoWindowText_disableTriggers(VuoWindowText w);
void VuoWindowText_appendLine(VuoWindowText w, const char *text);

VuoWindowOpenGl VuoWindowOpenGl_make
(
		bool useDepthBuffer,
		void (*initCallback)(VuoGlContext glContext, float backingScaleFactor, void *),
		void (*resizeCallback)(VuoGlContext glContext, void *, unsigned int, unsigned int),
		void (*drawCallback)(VuoGlContext glContext, void *),
		void *context
);
void VuoWindowOpenGl_enableTriggers
(
		VuoWindowOpenGl w,
		VuoOutputTrigger(showedWindow, VuoWindowReference)
);
void VuoWindowOpenGl_disableTriggers(VuoWindowOpenGl w);
void VuoWindowOpenGl_redraw(VuoWindowOpenGl w);
void VuoWindowOpenGl_setProperties(VuoWindowOpenGl w, VuoList_VuoWindowProperty properties);
void VuoWindowOpenGl_executeWithWindowContext(VuoWindowOpenGl w, void (^blockToExecute)(VuoGlContext glContext));
void VuoWindowOpenGl_setAspectRatio(VuoWindowOpenGl w, unsigned int pixelsWide, unsigned int pixelsHigh);
void VuoWindowOpenGl_unlockAspectRatio(VuoWindowOpenGl w);

#ifdef __cplusplus
}
#endif
