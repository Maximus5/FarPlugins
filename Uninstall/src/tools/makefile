CPPFLAGS = -nologo -EHsc -I..\common user32.lib advapi32.lib

build:
  $(CPP) $(CPPFLAGS) gendep.cpp
  $(CPP) $(CPPFLAGS) msgc.cpp
  $(CPP) $(CPPFLAGS) preproc.cpp
  $(CPP) $(CPPFLAGS) farver.cpp
  $(CPP) $(CPPFLAGS) convcp.cpp
  del /q *.obj
