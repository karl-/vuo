/**
 * @file
 * vuo.text.format.number node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoNumberFormat.h"

VuoModuleMetadata({
					  "title" : "Format Number",
					  "keywords" : [ "currency", "dollar", "euro", "percentage", "round" ],
					  "version" : "1.0.0",
					  "dependencies" : [ ],
					  "node" : {
						  "exampleCompositions" : [ ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoReal, {"default":"1.0"}) value,
	VuoInputData(VuoNumberFormat, {"default":"decimal"}) format,
	VuoInputData(VuoInteger, {"default":2, "suggestedMin": 0}) decimalPlaces,
	VuoInputData(VuoBoolean, {"default":true}) showThousandSeparator,
	VuoOutputData(VuoText) text
)
{
	CFNumberFormatterStyle style = kCFNumberFormatterDecimalStyle;
	if (format == VuoNumberFormat_Percentage)
		style = kCFNumberFormatterPercentStyle;
	else if (format == VuoNumberFormat_Currency)
		style = kCFNumberFormatterCurrencyStyle;

	CFLocaleRef currentLocale = CFLocaleCopyCurrent();
	CFNumberFormatterRef numberFormatter = CFNumberFormatterCreate(NULL, currentLocale, style);

	{
		CFNumberRef maxFractionDigits = CFNumberCreate(NULL, kCFNumberIntType, &decimalPlaces);
		CFNumberFormatterSetProperty(numberFormatter, kCFNumberFormatterMaxFractionDigits, maxFractionDigits);
		CFRelease(maxFractionDigits);
	}

	CFNumberFormatterSetProperty(numberFormatter, kCFNumberFormatterUseGroupingSeparator, showThousandSeparator ? kCFBooleanTrue : kCFBooleanFalse);

	{
		CFStringRef formattedNumberString = CFNumberFormatterCreateStringWithValue(NULL, numberFormatter, kCFNumberDoubleType, &value);
		*text = VuoText_makeFromCFString(formattedNumberString);
		CFRelease(formattedNumberString);
	}

	CFRelease(numberFormatter);
	CFRelease(currentLocale);
}
