
#define MVV_1 0
#define MVV_2 8
#define MVV_3 14
#define MVV_4 0
#define MVV_4a ""


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if (MVV_3 > 0)
  #define IMPEXVERS STRING(MVV_1) "." STRING(MVV_2) "." STRING(MVV_3)
#else
  #define IMPEXVERS STRING(MVV_1) "." STRING(MVV_2)
#endif
// "." STRING(MVV_3) MVV_4a
#define IMPEXVERN MVV_1,MVV_2,MVV_3,MVV_4

#ifdef _UNICODE
	#define IMPEXCHAR "Unicode"
#else
	#define IMPEXCHAR "Ansi"
#endif

#ifdef _WIN64
#define IMPEXPLTFRM " (" IMPEXCHAR " x64)"
#else
#define IMPEXPLTFRM " (" IMPEXCHAR " x86)"
#endif
