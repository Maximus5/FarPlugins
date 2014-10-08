
#define MVV_1 1
#define MVV_2 10
#define MVV_3 0
#define MVV_4 FARMANAGERVERSION_BUILD
#define MVV_4a ""

#include "../../common/plugin.h"


#define STRING2(x) #x
#define STRING(x) STRING2(x)

//#if (MVV_2<10)
//  #define MVV_2s "0" STRING(MVV_2)
//#else
//  #define MVV_2s STRING(MVV_2)
//#endif
//#if (MVV_3<10)
//  #define MVV_3s "0" STRING(MVV_3)
//#else
//  #define MVV_3s STRING(MVV_3)
//#endif
//#if (MVV_4>0)
//  #define MVV_3n MVV_4##MVV_3
//#else
//  #define MVV_3n MVV_3
//#endif

//#define EDITWRAPVERS STRING(MVV_1) "." STRING(MVV_2) "." STRING(MVV_3) MVV_4a
#define EDITWRAPVERS STRING(MVV_1) "." STRING(MVV_2) "#" STRING(MVV_4)

#ifdef __GNUC__
#define EDITWRAPVERN MVV_1,MVV_2,MVV_3,MVV_4
#else
//#define EDITWRAPVERN MVV_1,MVV_2,MVV_3n,MVV_4
#define EDITWRAPVERN MVV_1,MVV_2,MVV_3,MVV_4
#endif

#ifdef _WIN64
#define EDITWRAPPLTFRM " (x64)"
#else
#ifdef WIN64
#define EDITWRAPPLTFRM " (x64)"
#else
#define EDITWRAPPLTFRM " (x86)"
#endif
#endif
