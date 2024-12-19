// AwknCore.cpp : Defines the exported functions for the DLL.
//

#include "AwknCore.h"


// This is an example of an exported variable
AWKNCORE_API int nAwknCore=0;

// This is an example of an exported function.
AWKNCORE_API int fnAwknCore(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CAwknCore::CAwknCore()
{
    return;
}
