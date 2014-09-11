Synopsis
========
This is another PE browser.

Open dll, exe, lib, obj, icl, fon and other PE files (16/32/64 bit).

You may call plugin from Plugin menu (F11), too.

Plugin may be called by:
1) Command line prefix "ImpEx:<PE_file>"
2) Plugin menu, item "ImpEx"


UPX and other packers
=====================
Some PE files may be packed (by UPX, for example), in that case
you may see file like '32BIT,UPX' at the root of plugin.

If you want to see 'real' file - execute smth. like this:

upx.exe -d my_exe_file.exe

!!! Warning !!! THIS COMMAND UNPACK FILE IN PLACE !!!

You may download UPX here:
http://upx.sourceforge.net/#download
