
#define MVV_1 3
#define MVV_2 1
#define MVV_3 1
#define MVV_4 0
#define MVV_4a ""


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define DIZC0VERS STRING(MVV_1) "." STRING(MVV_2) "." STRING(MVV_3)
#define DIZC0VERN MVV_1,MVV_2,MVV_3,MVV_4

#ifdef _WIN64
#define DIZC0PLTFRM " (x64)"
#else
#define DIZC0PLTFRM " (x86)"
#endif
