/**
 * @file
 * VuoInputEditorWithEnumMenu implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoInputEditorWithEnumMenu.hh"

#include <dlfcn.h>

/**
 * Creates an enum input editor for the specified Vuo Type.
 *
 * @param type e.g., `VuoBoolean`
 */
VuoInputEditorWithEnumMenu::VuoInputEditorWithEnumMenu(QString type)
{
	this->type = type;
}

/**
 * Creates the tree that models the menu.
 */
VuoInputEditorMenuItem * VuoInputEditorWithEnumMenu::setUpMenuTree(json_object *details)
{
	VuoInputEditorMenuItem *rootMenuItem = new VuoInputEditorMenuItem("root");

	QString allowedValuesFunctionName = this->type + "_getAllowedValues";
	typedef void *(*allowedValuesFunctionType)(void);
	allowedValuesFunctionType allowedValuesFunction = (allowedValuesFunctionType)dlsym(RTLD_SELF, allowedValuesFunctionName.toUtf8().constData());

	QString summaryFunctionName = this->type + "_getSummary";
	typedef char *(*summaryFunctionType)(unsigned long);
	summaryFunctionType summaryFunction = (summaryFunctionType)dlsym(RTLD_SELF, summaryFunctionName.toUtf8().constData());

	QString jsonFunctionName = this->type + "_getJson";
	typedef json_object *(*jsonFunctionType)(unsigned long);
	jsonFunctionType jsonFunction = (jsonFunctionType)dlsym(RTLD_SELF, jsonFunctionName.toUtf8().constData());

	QString listCountFunctionName = "VuoListGetCount_" + this->type;
	typedef unsigned long (*listCountFunctionType)(void *);
	listCountFunctionType listCountFunction = (listCountFunctionType)dlsym(RTLD_SELF, listCountFunctionName.toUtf8().constData());

	QString listValueFunctionName = "VuoListGetValue_" + this->type;
	typedef unsigned long (*listValueFunctionType)(void *, unsigned long);
	listValueFunctionType listValueFunction = (listValueFunctionType)dlsym(RTLD_SELF, listValueFunctionName.toUtf8().constData());

	if (allowedValuesFunction && summaryFunction && jsonFunction && listCountFunction && listValueFunction)
	{
		void *allowedValues = allowedValuesFunction();
		unsigned long count = listCountFunction(allowedValues);
		for (unsigned long i=1; i<=count; ++i)
		{
			unsigned long value = listValueFunction(allowedValues, i);
			json_object *valueJson = jsonFunction(value);
			if (!shouldIncludeValue(valueJson))
				continue;

			char *summary = summaryFunction(value);
			rootMenuItem->addItem(new VuoInputEditorMenuItem(summary, valueJson));
			free(summary);
		}
	}

	return rootMenuItem;
}
