/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Grendel mail/news client.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1997
 * Netscape Communications Corporation.  All Rights Reserved.
 */

package calypso.util;

public class CharArray
{
  public static int indexOf(char[] str, int ch, int fromIndex)
  {
    int len = str.length;
    for (int i = fromIndex; i < len; i++)
    {
      if (str[i] == ch)
      {
        return i;
      }
    }
    return -1;
  }

  public static int indexOf(char[] str, int ch)
  {
    return indexOf(str, ch, 0);
  }
}
