/*
The contents of this file are subject to the Mozilla Public License
Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations
under the License.

The Original Code is expat.

The Initial Developer of the Original Code is James Clark.
Portions created by James Clark are Copyright (C) 1998
James Clark. All Rights Reserved.

Contributor(s):
*/

#include "codepage.h"

#ifdef WIN32
#include <windows.h>

int codepageMap(int cp, unsigned short *map)
{
  int i;
  CPINFO info;
  if (!GetCPInfo(cp, &info) || info.MaxCharSize > 2)
    return 0;
  for (i = 0; i < 256; i++)
    map[i] = 0;
  if (info.MaxCharSize > 1) {
    for (i = 0; i < MAX_LEADBYTES; i++) {
      int j, lim;
      if (info.LeadByte[i] == 0 && info.LeadByte[i + 1] == 0)
        break;
      lim = info.LeadByte[i + 1];
      for (j = info.LeadByte[i]; j < lim; j++)
	map[j] = 2;
    }
  }
  for (i = 0; i < 256; i++) {
   if (map[i] == 0) {
     char c = i;
     if (MultiByteToWideChar(cp, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
		             &c, 1, map + i, 1) == 0)
       map[i] = 0;
   }
  }
  return 1;
}

unsigned short codepageConvert(int cp, const char *p)
{
  unsigned short c;
  if (MultiByteToWideChar(cp, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
		          p, 2, &c, 1) == 1)
    return c;
  return 0;
}

#else /* not WIN32 */

int codepageMap(int cp, unsigned short *map)
{
  return 0;
}

unsigned short codepageConvert(int cp, const char *p)
{
  return 0;
}

#endif /* not WIN32 */
