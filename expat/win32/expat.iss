;  Basic setup script for the Inno Setep installer builder.  For more
;  information on the free installer builder, see www.jrsoftware.org.
;
;  This script was contributed by Tim Peters.

[Setup]
AppName=expat
AppId=expat
AppVersion=1.95.2
AppVerName=expat 1.95.2
AppCopyright=Copyright © 1998-2001 Thai Open Source Software Center and Clark Cooper
DefaultDirName={sd}\Expat-1.95.2
AppPublisher=PythonLabs at Zope Corporation
AppPublisherURL=http://www.python.org
AppSupportURL=http://www.python.org
AppUpdatesURL=http://www.python.org

DisableStartupPrompt=yes
AllowNoIcons=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes

[Files]
Source: *.h;     DestDir: "{app}";  CopyMode: alwaysoverwrite
Source: *.dll;   DestDir: "{app}";  CopyMode: alwaysoverwrite
Source: *.lib;   DestDir: "{app}";  CopyMode: alwaysoverwrite
Source: *.html;  DestDir: "{app}";  CopyMode: alwaysoverwrite
Source: *.css;   DestDir: "{app}";  CopyMode: alwaysoverwrite
Source: *.exe;   DestDir: "{app}";  CopyMode: alwaysoverwrite
Source: COPYING; DestDir: "{app}";  DestName: COPYING.txt;  CopyMode: alwaysoverwrite
Source: README;  DestDir: "{app}";  DestName: README.txt;   CopyMode: alwaysoverwrite
