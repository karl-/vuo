/**
 * @file
 * VuoAnchor C type definition.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoHorizontalAlignment.h"
#include "VuoVerticalAlignment.h"

/// @{
typedef const struct VuoList_VuoAnchor_struct { void *l; } * VuoList_VuoAnchor;
#define VuoList_VuoAnchor_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoAnchor VuoAnchor
 * Combination vertical + horizontal alignment.
 *
 * @{
 */

/**
 * Combination vertical + horizontal alignment.
 *
 * (VuoVerticalAlignment << 2) + VuoHorizontalAlignment
 */
typedef int64_t VuoAnchor;

VuoAnchor VuoAnchor_makeFromJson(struct json_object * js);
struct json_object * VuoAnchor_getJson(const VuoAnchor value);
char * VuoAnchor_getSummary(const VuoAnchor value);
VuoList_VuoAnchor VuoAnchor_getAllowedValues(void);

/**
 * Returns a VuoAnchor with horizontal and vertical alignments.
 */
static inline VuoAnchor VuoAnchor_make(VuoHorizontalAlignment horizontal, VuoVerticalAlignment vertical) __attribute__((const));
static inline VuoAnchor VuoAnchor_make(VuoHorizontalAlignment horizontal, VuoVerticalAlignment vertical)
{
	return (vertical << 2) + horizontal;
}

/**
 * Returns the horizontal component of a VuoAnchor.
 */
static inline VuoHorizontalAlignment VuoAnchor_getHorizontal(VuoAnchor anchor) __attribute__((const));
static inline VuoHorizontalAlignment VuoAnchor_getHorizontal(VuoAnchor anchor)
{
	return (VuoHorizontalAlignment)(anchor & 0x3);
}

/**
 * Returns the horizontal component of a VuoAnchor.
 */
static inline VuoVerticalAlignment VuoAnchor_getVertical(VuoAnchor anchor) __attribute__((const));
static inline VuoVerticalAlignment VuoAnchor_getVertical(VuoAnchor anchor)
{
	return (VuoVerticalAlignment)((anchor >> 2) & 0x3);
}

/**
 * Returns a VuoAnchor with both horizontal and vertical alignments centered.
 */
static inline VuoAnchor VuoAnchor_makeCentered(void) __attribute__((const));
static inline VuoAnchor VuoAnchor_makeCentered(void)
{
	return VuoAnchor_make(VuoHorizontalAlignment_Center, VuoVerticalAlignment_Center);
}

/**
 * Returns true if the two values are equal.
 */
static inline bool VuoAnchor_areEqual(const VuoAnchor value1, const VuoAnchor value2)
{
	return VuoAnchor_getHorizontal(value1) == VuoAnchor_getHorizontal(value2)
		&& VuoAnchor_getVertical(value1)   == VuoAnchor_getVertical(value2);
}

/**
 * Returns true if the value1 is less than value2.
 */
static inline bool VuoAnchor_isLessThan(const VuoAnchor value1, const VuoAnchor value2)
{
	return VuoHorizontalAlignment_isLessThan(VuoAnchor_getHorizontal(value1), VuoAnchor_getHorizontal(value2))
		&& VuoVerticalAlignment_isLessThan(VuoAnchor_getVertical(value1), VuoAnchor_getVertical(value2));
}

/**
 * Automatically generated function.
 */
///@{
VuoAnchor VuoAnchor_makeFromString(const char *str);
char * VuoAnchor_getString(const VuoAnchor value);
void VuoAnchor_retain(VuoAnchor value);
void VuoAnchor_release(VuoAnchor value);
///@}

/**
 * @}
 */


