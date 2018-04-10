#!/bin/bash

CORETYPES_HEADER=coreTypes.h

echo '// This header is generated by vuo/type/generateCoreTypesHeader.sh.' > $CORETYPES_HEADER

for header in $* ; do
	TYPE=${header%.h}
	echo "#include \"$header\"" >> $CORETYPES_HEADER

	if [[ $header != VuoDictionary_* ]] && [[ $header != "VuoMathExpressionList.h" ]] ; then
		TYPE=VuoList_${header%.h}
		echo "#include \"VuoList_$header\"" >> $CORETYPES_HEADER
	fi
done
