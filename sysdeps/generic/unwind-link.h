/* Dynamic loading of the libgcc unwinder.  Generic version.
   Copyright (C) 2020 Free Software Foundation, Inc.
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

#ifndef _UNWIND_LINK_H
#define _UNWIND_LINK_H

#include <unwind.h>

#ifdef SHARED

#include <sysdep.h>
#include <unwind-resume.h>

struct unwind_link
{
  void *libgcc_handle;
  __typeof (_Unwind_Backtrace) *ptr__Unwind_Backtrace;
  __typeof (_Unwind_ForcedUnwind) *ptr__Unwind_ForcedUnwind;
  __typeof (_Unwind_GetCFA) *ptr__Unwind_GetCFA;
# ifndef UNWIND_LINK_NO_GETIP
  __typeof (_Unwind_GetIP) *ptr__Unwind_GetIP;
# endif
  __typeof (_Unwind_Resume) *ptr__Unwind_Resume;
  _Unwind_Reason_Code (*ptr_personality) PERSONALITY_PROTO;
# ifdef UNWIND_LINK_EXTRA_FIELDS
  UNWIND_LINK_EXTRA_FIELDS
# endif
};

/* Return a pointer to the implementation, or NULL on failure.  */
struct unwind_link *__libc_unwind_link_get (void);
libc_hidden_proto (__libc_unwind_link_get)

/* UNWIND_LINK_CALL invokes the stored function pointer NAME via the
   cached unwind link OBJ (which was previously returned by
   __libc_unwind_link_get).  */
# ifdef PTR_DEMANGLE
#  define UNWIND_LINK_CALL(obj, name, ...)                          \
  ({                                                                \
    __typeof ((obj)->ptr_##name) __unwind_fptr = (obj)->ptr_##name; \
    PTR_DEMANGLE (__unwind_fptr);                                   \
    __unwind_fptr (__VA_ARGS__);                                    \
  })
# else /* !PTR_DEMANGLE */
#  define UNWIND_LINK_CALL(obj, name, ...) ((obj)->ptr_##name (__VA_ARGS__))
# endif

/* Called from __libc_freeres.  */
void __libc_unwind_link_freeres (void) attribute_hidden;

#else /* !SHARED */

/* Dummy implementation so that the code can be shared with the SHARED
   version.  */
struct unwind_link;
static inline struct unwind_link *
__libc_unwind_link_get (void)
{
  /* Return something that is not a null pointer, so that error checks
     succeed.  */
  return (struct unwind_link *) 1;
}

/* Directly call the static implementation.  */
# define UNWIND_LINK_CALL(obj, name, ...) \
  ((void) (obj), (name (__VA_ARGS__)))

#endif /* !SHARED */

#endif /* _UNWIND_LINK_H */
