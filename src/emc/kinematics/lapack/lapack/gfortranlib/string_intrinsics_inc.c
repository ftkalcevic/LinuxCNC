/* String intrinsics helper functions.
   Copyright 2002, 2005, 2007, 2008, 2009 Free Software Foundation, Inc.

This file is part of the GNU Fortran runtime library (libgfortran).

Libgfortran is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

Libgfortran is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */


/* Rename the functions.  */
#define concat_string SUFFIX(concat_string)
#define string_len_trim SUFFIX(string_len_trim)
#define compare_string SUFFIX(compare_string)


/* Use for functions which can return a zero-length string.  */
static CHARTYPE zero_length_string = 0;


/* Strings of unequal length are extended with pad characters.  */

int
compare_string (gfc_charlen_type len1, const CHARTYPE *s1,
		gfc_charlen_type len2, const CHARTYPE *s2)
{
  const UCHARTYPE *s;
  gfc_charlen_type len;
  int res;

  res = memcmp (s1, s2, ((len1 < len2) ? len1 : len2) * sizeof (CHARTYPE));
  if (res != 0)
    return res;

  if (len1 == len2)
    return 0;

  if (len1 < len2)
    {
      len = len2 - len1;
      s = (UCHARTYPE *) &s2[len1];
      res = -1;
    }
  else
    {
      len = len1 - len2;
      s = (UCHARTYPE *) &s1[len2];
      res = 1;
    }

  while (len--)
    {
      if (*s != ' ')
        {
          if (*s > ' ')
            return res;
          else
            return -res;
        }
      s++;
    }

  return 0;
}
iexport(compare_string);


/* The destination and source should not overlap.  */

void
concat_string (gfc_charlen_type destlen, CHARTYPE * dest,
	       gfc_charlen_type len1, const CHARTYPE * s1,
	       gfc_charlen_type len2, const CHARTYPE * s2)
{
  if (len1 >= destlen)
    {
      memcpy (dest, s1, destlen * sizeof (CHARTYPE));
      return;
    }
  memcpy (dest, s1, len1 * sizeof (CHARTYPE));
  dest += len1;
  destlen -= len1;

  if (len2 >= destlen)
    {
      memcpy (dest, s2, destlen * sizeof (CHARTYPE));
      return;
    }

  memcpy (dest, s2, len2 * sizeof (CHARTYPE));
  MEMSET (&dest[len2], ' ', destlen - len2);
}

/* The length of a string not including trailing blanks.  */

gfc_charlen_type
string_len_trim (gfc_charlen_type len, const CHARTYPE *s)
{
  gfc_charlen_type i;

  for (i = len - 1; i >= 0; i--)
    {
      if (s[i] != ' ')
        break;
    }
  return i + 1;
}

