
#define MVV_1 1
#define MVV_2 0
#define MVV_3 0
#define MVV_4 0
#define MVV_4a ""


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define BASE64VERS STRING(MVV_1) "." STRING(MVV_2) "." STRING(MVV_3) MVV_4a
#define BASE64VERN MVV_1,MVV_2,MVV_3,MVV_4

#ifdef _WIN64
#define BASE64PLTFRM " (x64)"
#else
#define BASE64PLTFRM " (x86)"
#endif
