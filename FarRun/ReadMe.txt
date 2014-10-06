Useful for running Far as git editor.

It's unfriendly to run something long from command line (and from git's core.editor).
For example, my Far Manager instance is started like this:

    %ConEmuDrive%\Far30.Latest\far-bis\Far.exe /w /x /p%ConEmuDrive%\Far3\Plugins\ConEmu;%ConEmuDrive%\Far30.Latest\far-bis\Plugins;%ConEmuDrive%\Far30.Latest\Plugins;%ConEmuDrive%\Far30.Latest\Plugins.My

So, now the life will be easier.
1) Just copy FarRun.exe to any folder in %PATH%.
2) Create text file FarRun.ini with following

    [Run]
    Cmd=%ConEmuDrive%\Far30.Latest\far-bis\Far.exe /w /x /p%ConEmuDrive%\Far3\Plugins\ConEmu;%ConEmuDrive%\Far30.Latest\far-bis\Plugins;%ConEmuDrive%\Far30.Latest\Plugins;%ConEmuDrive%\Far30.Latest\Plugins.My

3) Update your core.editor

    git config core.editor "farrun -e1:1"

4) Enjoy the power and flexibility :P
