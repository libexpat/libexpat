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

int codepage(int cp, unsigned short *map)
{
  int i;
  CPINFO info;
  if (!GetCPInfo(cp, &info) || info.MaxCharSize > 1)
    return 0;
  for (i = 0; i < 256; i++) {
    char c = i;
    if (MultiByteToWideChar(cp, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
			    &c, 1, map + i, 1) == 0)
      map[i] = 0;
  }
  return 1;
}

#else /* not WIN32 */

int codepage(int cp, unsigned short *map)
{
  return 0;
}

#endif /* not WIN32 */
