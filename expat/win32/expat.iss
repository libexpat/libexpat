; Basic setup script for the Inno Setep installer builder.  For more
; information on the free installer builder, see www.jrsoftware.org.
;
; This script was contributed by Tim Peters.

[Setup]
AppName=expat
AppId=expat
AppVersion=1.95.2
AppVerName=expat 1.95.2
AppCopyright=Copyright © 1998-2001 Thai Open Source Software Center and Clark Cooper
DefaultDirName={sd}\Expat-1.95.2
AppPublisher=The Expat Developers
AppPublisherURL=http://expat.sourceforge.net/
AppSupportURL=http://expat.sourceforge.net/
AppUpdatesURL=http://expat.sourceforge.net/
SourceDir=..

DisableStartupPrompt=yes
AllowNoIcons=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes

[Files]
Source: lib\*.c;             DestDir: "{app}\Source";  CopyMode: alwaysoverwrite
Source: lib\*.h;             DestDir: "{app}\Source";  CopyMode: alwaysoverwrite
Source: lib\*.dsp;           DestDir: "{app}\Source";  CopyMode: alwaysoverwrite
Source: lib\Release\*.dll;   DestDir: "{app}\Libs";    CopyMode: alwaysoverwrite
Source: lib\Release\*.lib;   DestDir: "{app}\Libs";    CopyMode: alwaysoverwrite
Source: doc\*.html;          DestDir: "{app}\Doc";     CopyMode: alwaysoverwrite
Source: doc\*.css;           DestDir: "{app}\Doc";     CopyMode: alwaysoverwrite
Source: xmlwf\Release\*.exe; DestDir: "{app}";         CopyMode: alwaysoverwrite
Source: win32\MANIFEST.txt;  DestDir: "{app}";         CopyMode: alwaysoverwrite
Source: COPYING;             DestDir: "{app}";         CopyMode: alwaysoverwrite; DestName: COPYING.txt
Source: README;              DestDir: "{app}";         CopyMode: alwaysoverwrite; DestName: README.txt

