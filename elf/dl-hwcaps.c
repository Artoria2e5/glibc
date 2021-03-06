/* Hardware capability support for run-time dynamic loader.
   Copyright (C) 2012-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <libintl.h>
#include <unistd.h>
#include <ldsodefs.h>

#include <dl-procinfo.h>
#include <dl-hwcaps.h>

/* Return an array of useful/necessary hardware capability names.  */
const struct r_strlenpair *
_dl_important_hwcaps (const char *platform, size_t platform_len, size_t *sz,
		      size_t *max_capstrlen)
{
  uint64_t hwcap_mask = GET_HWCAP_MASK();
  /* Determine how many important bits are set.  */
  uint64_t masked = GLRO(dl_hwcap) & hwcap_mask;
  size_t cnt = platform != NULL;
  size_t n, m;
  size_t total;
  struct r_strlenpair *result;
  struct r_strlenpair *rp;
  char *cp;

  /* Count the number of bits set in the masked value.  */
  for (n = 0; (~((1ULL << n) - 1) & masked) != 0; ++n)
    if ((masked & (1ULL << n)) != 0)
      ++cnt;

  /* For TLS enabled builds always add 'tls'.  */
  ++cnt;

  /* Create temporary data structure to generate result table.  */
  struct r_strlenpair temp[cnt];
  m = 0;
  for (n = 0; masked != 0; ++n)
    if ((masked & (1ULL << n)) != 0)
      {
	temp[m].str = _dl_hwcap_string (n);
	temp[m].len = strlen (temp[m].str);
	masked ^= 1ULL << n;
	++m;
      }
  if (platform != NULL)
    {
      temp[m].str = platform;
      temp[m].len = platform_len;
      ++m;
    }

  temp[m].str = "tls";
  temp[m].len = 3;
  ++m;

  assert (m == cnt);

  /* Determine the total size of all strings together.  */
  if (cnt == 1)
    total = temp[0].len + 1;
  else
    {
      total = temp[0].len + temp[cnt - 1].len + 2;
      if (cnt > 2)
	{
	  total <<= 1;
	  for (n = 1; n + 1 < cnt; ++n)
	    total += temp[n].len + 1;
	  if (cnt > 3
	      && (cnt >= sizeof (size_t) * 8
		  || total + (sizeof (*result) << 3)
		     >= (1UL << (sizeof (size_t) * 8 - cnt + 3))))
	    _dl_signal_error (ENOMEM, NULL, NULL,
			      N_("cannot create capability list"));

	  total <<= cnt - 3;
	}
    }

  /* The result structure: we use a very compressed way to store the
     various combinations of capability names.  */
  *sz = 1 << cnt;
  result = (struct r_strlenpair *) malloc (*sz * sizeof (*result) + total);
  if (result == NULL)
    _dl_signal_error (ENOMEM, NULL, NULL,
		      N_("cannot create capability list"));

  if (cnt == 1)
    {
      result[0].str = (char *) (result + *sz);
      result[0].len = temp[0].len + 1;
      result[1].str = (char *) (result + *sz);
      result[1].len = 0;
      cp = __mempcpy ((char *) (result + *sz), temp[0].str, temp[0].len);
      *cp = '/';
      *sz = 2;
      *max_capstrlen = result[0].len;

      return result;
    }

  /* Fill in the information.  This follows the following scheme
     (indices from TEMP for four strings):
	entry #0: 0, 1, 2, 3	binary: 1111
	      #1: 0, 1, 3		1101
	      #2: 0, 2, 3		1011
	      #3: 0, 3			1001
     This allows the representation of all possible combinations of
     capability names in the string.  First generate the strings.  */
  result[1].str = result[0].str = cp = (char *) (result + *sz);
#define add(idx) \
      cp = __mempcpy (__mempcpy (cp, temp[idx].str, temp[idx].len), "/", 1);
  if (cnt == 2)
    {
      add (1);
      add (0);
    }
  else
    {
      n = 1 << (cnt - 1);
      do
	{
	  n -= 2;

	  /* We always add the last string.  */
	  add (cnt - 1);

	  /* Add the strings which have the bit set in N.  */
	  for (m = cnt - 2; m > 0; --m)
	    if ((n & (1 << m)) != 0)
	      add (m);

	  /* Always add the first string.  */
	  add (0);
	}
      while (n != 0);
    }
#undef add

  /* Now we are ready to install the string pointers and length.  */
  for (n = 0; n < (1UL << cnt); ++n)
    result[n].len = 0;
  n = cnt;
  do
    {
      size_t mask = 1 << --n;

      rp = result;
      for (m = 1 << cnt; m > 0; ++rp)
	if ((--m & mask) != 0)
	  rp->len += temp[n].len + 1;
    }
  while (n != 0);

  /* The first half of the strings all include the first string.  */
  n = (1 << cnt) - 2;
  rp = &result[2];
  while (n != (1UL << (cnt - 1)))
    {
      if ((--n & 1) != 0)
	rp[0].str = rp[-2].str + rp[-2].len;
      else
	rp[0].str = rp[-1].str;
      ++rp;
    }

  /* The second half starts right after the first part of the string of
     the corresponding entry in the first half.  */
  do
    {
      rp[0].str = rp[-(1 << (cnt - 1))].str + temp[cnt - 1].len + 1;
      ++rp;
    }
  while (--n != 0);

  /* The maximum string length.  */
  *max_capstrlen = result[0].len;

  return result;
}
