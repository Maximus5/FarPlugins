
#define MVV_1 1
#define MVV_2 0
#define MVV_3 1
#define MVV_4 0
#define MVV_4a ""


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#define MYDLLVERS STRING(MVV_1) "." STRING(MVV_2) "." STRING(MVV_3) MVV_4a "\0"
#define MYDLLVERN MVV_1,MVV_2,MVV_3,MVV_4

#ifdef _WIN64
#define MYDLLPLTFRM " (x64)"
#else
#define MYDLLPLTFRM " (x86)"
#endif
