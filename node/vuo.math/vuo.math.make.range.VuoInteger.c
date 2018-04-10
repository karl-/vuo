/**
 * @file
 * vuo.math.make.range node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Range (Integer)",
					  "keywords" : [ ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoBoolean, {"default":true}) hasMinimum,
		VuoInputData(VuoInteger, {"default":1}) minimum,
		VuoInputData(VuoBoolean, {"default":true}) hasMaximum,
		VuoInputData(VuoInteger, {"default":10}) maximum,
		VuoOutputData(VuoIntegerRange) range
)
{
	*range = VuoIntegerRange_make(hasMinimum ? minimum : VuoIntegerRange_NoMinimum,
								  hasMaximum ? maximum : VuoIntegerRange_NoMaximum);
}