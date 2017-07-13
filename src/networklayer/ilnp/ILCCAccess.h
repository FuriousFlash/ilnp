
// @kbteja...

#ifndef __ILCCACCESS_H__
#define __ILCCACCESS_H__



#include "INETDefs.h"

#include "ModuleAccess.h"
#include "ILCC.h"


/**
 * Give the access of ilcc module to another modules which try to access it.
 */

class INET_API ILCCAccess : public ModuleAccess<ILCC>
{
    public:
       ILCCAccess() : ModuleAccess<ILCC>("ilcc") {}
};

#endif
