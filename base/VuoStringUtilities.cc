/**
 * @file
 * VuoStringUtilities implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoStringUtilities.hh"

extern "C" {
#include "mkdio.h"
}

/**
 * Returns true if @c wholeString begins with @c beginning.
 */
bool VuoStringUtilities::beginsWith(string wholeString, string beginning)
{
	return wholeString.length() >= beginning.length() && wholeString.substr(0, beginning.length()) == beginning;
}

/**
 * Returns true if @c wholeString ends with @c ending.
 */
bool VuoStringUtilities::endsWith(string wholeString, string ending)
{
	if (wholeString.length() < ending.length())
		return false;

	return wholeString.compare(wholeString.length()-ending.length(), ending.length(), ending) == 0;
}

/**
 * Returns the substring of @c wholeString that follows @c beginning,
 * or an empty string if @c wholeString does not begin with @c beginning.
 */
string VuoStringUtilities::substrAfter(string wholeString, string beginning)
{
	if (! beginsWith(wholeString, beginning))
		return "";

	return wholeString.substr(beginning.length());
}

/**
 * Returns a string constructed by replacing all instances of @c originalChar with
 * @c replacementChar in @c wholeString.
 */
string VuoStringUtilities::replaceAll(string wholeString, char originalChar, char replacementChar)
{
	string outString = wholeString;
	size_t pos = 0;
	while ((pos = wholeString.find_first_of(originalChar, pos)) != string::npos)
	{
		outString[pos] = replacementChar;
		pos = pos + 1;
	}
	return outString;
}

/**
 * Transforms a string into a valid identifier:
 *  - Replace whitespace and '.'s with '_'s
 *  - Make sure all characters match [A-Za-z0-9_]
 */
string VuoStringUtilities::transcodeToIdentifier(string str)
{
	string sanitizedStr = str;
	int strLen = sanitizedStr.length();
	for (int i=0; i<strLen; i++) {
		if (sanitizedStr[i] == '.' || isspace(sanitizedStr[i])) {sanitizedStr[i] = '_';}
		else if (!(isValidCharInIdentifier(sanitizedStr[i]))) {
			// For now, replace invalid characters with '0'
			sanitizedStr[i] = '0';
		}
	}
	return sanitizedStr;
}

/**
 * Check whether a character is valid for an
 * identifier, i.e., matches [A-Za-z0-9_]
 */
bool VuoStringUtilities::isValidCharInIdentifier(char ch)
{
	bool valid = (isalnum(ch) || ch=='_');
	return valid;
}

/**
 * Escapes backslashes, quotes, angle brackets, and pipes in the string, making it a valid identifier in the Graphviz DOT language.
 */
string VuoStringUtilities::transcodeToGraphvizIdentifier(const string &originalString)
{
	string escapedString = originalString;
	for (string::size_type i = 0; (i = escapedString.find("\\", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\\\");
	for (string::size_type i = 0; (i = escapedString.find("\"", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\\"");
	for (string::size_type i = 0; (i = escapedString.find("<", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\<");
	for (string::size_type i = 0; (i = escapedString.find(">", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\>");
	for (string::size_type i = 0; (i = escapedString.find("|", i)) != std::string::npos; i += 2)
		escapedString.replace(i, 1, "\\|");
	return escapedString;
}

/**
 * Removes escapes for backslashes, quotes, angle brackets, and pipes in the Graphviz identifier, making it a normal string.
 */
string VuoStringUtilities::transcodeFromGraphvizIdentifier(const string &graphvizIdentifier)
{
	string unescapedString;
	bool inEscape = false;
	for (string::const_iterator i = graphvizIdentifier.begin(); i != graphvizIdentifier.end(); ++i)
	{
		if (inEscape)
		{
			inEscape = false;
			unescapedString += *i;
			continue;
		}

		if (*i == '\\')
		{
			inEscape = true;
			continue;
		}

		unescapedString  += *i;
	}
	return unescapedString;
}

/**
 * Converts @c markdownString to HTML.
 */
string VuoStringUtilities::generateHtmlFromMarkdown(const string &markdownString)
{
	MMIOT *doc = mkd_string(markdownString.c_str(), markdownString.length(), 0);
	mkd_compile(doc, 0);
	char *html;
	mkd_document(doc, &html);
	string htmlString(html);
	return htmlString;
}
