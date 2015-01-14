/**
 * @file
 * vuo.math.count.real node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <stdlib.h>

VuoModuleMetadata({
					 "title" : "Count",
					 "description" :
						"<p>Keeps track of a count that can be incremented and decremented.</p> \
						<p>Before this node has received any events that change the count, the count is 0.</p> \
						<p><ul> \
						<li>`increment` — When this port receives an event, the count is incremented by this port's value.</li> \
						<li>`decrement` — When this port receives an event, the count is decremented by this port's value.</li> \
						<li>`setCount` — When this port receives an event, the count is changed to this port's value. (If the same event arrives at this port and other ports, the count is set, and the increment or decrement is ignored.)</li> \
						</ul></p>",
					 "keywords" : [ "count", "add", "sum", "total", "tally", "integrator", "increment", "decrement", "increase", "decrease" ],
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });


VuoReal * nodeInstanceInit()
{
	VuoReal *countState = (VuoReal *) malloc(sizeof(VuoReal));
	VuoRegister(countState, free);
	*countState = 0;
	return countState;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoReal *) countState,
		VuoInputData(VuoReal, {"default":1}) increment,
		VuoInputEvent(VuoPortEventBlocking_None,increment) incrementEvent,
		VuoInputData(VuoReal, {"default":1}) decrement,
		VuoInputEvent(VuoPortEventBlocking_None,decrement) decrementEvent,
		VuoInputData(VuoReal, {"default":0}) setCount,
		VuoInputEvent(VuoPortEventBlocking_None,setCount) setCountEvent,
		VuoOutputData(VuoReal) count
)
{
	if (incrementEvent)
		**countState += increment;
	if (decrementEvent)
		**countState -= decrement;
	if (setCountEvent)
		**countState = setCount;
	*count = **countState;
}


void nodeInstanceFini
(
		VuoInstanceData(VuoReal *) countState
)
{
}