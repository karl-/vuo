/**
 * @file
 * vuo.vertices.make.points node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Point Mesh",
					  "keywords" : [ ],
					  "version" : "2.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "isInterface" : false
					  }
				  });

void nodeEvent
(
		VuoInputData(VuoList_VuoGenericType1) positions,
		VuoInputData(VuoReal, {"default":0.01, "suggestedMin":0.0, "suggestedStep":0.001}) pointSize,
		VuoOutputData(VuoMesh) mesh
)
{
	*mesh = VuoMesh_make_VuoGenericType1(positions, VuoMesh_Points, pointSize);
}