/**
 * @file
 * vuo.image.convolve node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageConvolve.h"
#include "VuoThresholdType.h"

VuoModuleMetadata({
					  "title" : "Outline Image",
					  "keywords" : [
						  "filter",
						  "edge detection", "convolution",
						  "Laplacian of Gaussian", "LoG", "Marr",
						  "emboss", "relief",
					  ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoImageConvolve"
					  ],
					  "node": {
						  "exampleCompositions" : [ "OutlineSeagulls.vuo" ]
					  }
				 });

VuoImageConvolve *nodeInstanceInit(void)
{
	return VuoImageConvolve_make();
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoImageConvolve *) convolve,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoThresholdType, {"default":"rgb"}) channels,
		VuoInputData(VuoReal, {"default":2, "suggestedMin":0, "suggestedMax":10}) radius,
		VuoInputData(VuoReal, {"default":1, "suggestedMin":-2, "suggestedMax":2}) intensity,
		VuoInputData(VuoDiode, {"default":"bipolar"}) range,
		VuoOutputData(VuoImage, {"name":"Outlined Image"}) convolvedImage
)
{
	VuoReal clampedRadius = VuoReal_makeNonzero(radius);
	unsigned int width = VuoImageConvolve_laplacianOfGaussianWidth(clampedRadius);
	VuoImage convolutionMatrix = VuoImageConvolve_generateMatrix(VuoImageConvolve_laplacianOfGaussian, width, true, clampedRadius);
	VuoLocal(convolutionMatrix);

	*convolvedImage = VuoImageConvolve_convolve(*convolve, image, convolutionMatrix, channels, intensity, range);
}

void nodeInstanceFini(VuoInstanceData(VuoImageConvolve *) convolve)
{
}