/* failmalloc - force to fail in allocating memory sometimes */
/*
 * Copyright (C) 2006 Yoshinori K. Okuji <okuji@enbug.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <malloc.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

static void failmalloc_install (void);
static void failmalloc_uninstall (void);

static void *(*old_malloc_hook) (size_t, const void *);
static void *(*old_realloc_hook) (void *ptr, size_t size, const void *);
static void *(*old_memalign_hook) (size_t, size_t, const void *);

static double failure_probability = 1.0;
static unsigned long failure_interval = 1;
static unsigned long count = 0;
static long max_failures = -1;
static size_t max_space = 0;
static size_t current_space = 0;

static int
should_fail (size_t size)
{
  if (max_failures == 0)
    return 0;

  if (max_space)
    {
      if (current_space < max_space - size)
	{
	  current_space += size;
	  return 0;
	}
    }
  
  if (failure_interval > 1)
    {
      count++;
      if (count < failure_interval)
        return 0;

      count = 0;
    }

  if (failure_probability == 1.0 || RAND_MAX * failure_probability >= rand ())
    goto fail;

  return 0;

 fail:
  
  if (max_failures > 0)
    max_failures--;
  
  return 1;
}

static void *
failmalloc (size_t size, const void *caller)
{
  void *ret;

  if (should_fail (size))
    {
      errno = ENOMEM;
      return NULL;
    }

  failmalloc_uninstall ();
  ret = malloc (size);
  failmalloc_install ();

  return ret;
}

static void *
failrealloc (void *ptr, size_t size, const void *caller)
{
  void *ret;

  if (should_fail (size))
    {
      errno = ENOMEM;
      return NULL;
    }

  failmalloc_uninstall ();
  ret = realloc (ptr, size);
  failmalloc_install ();

  return ret;
}

static void *
failmemalign (size_t alignment, size_t size, const void *caller)
{
  void *ret;

  if (should_fail (size))
    {
      errno = ENOMEM;
      return NULL;
    }

  failmalloc_uninstall ();
  ret = memalign (alignment, size);
  failmalloc_install ();

  return ret;
}

static void
failmalloc_install (void)
{
  old_malloc_hook = __malloc_hook;
  old_realloc_hook = __realloc_hook;
  old_memalign_hook = __memalign_hook;
  __malloc_hook = failmalloc;
  __realloc_hook = failrealloc;
  __memalign_hook = failmemalign;
}

static void
failmalloc_uninstall (void)
{
  __malloc_hook = old_malloc_hook;
  __realloc_hook = old_realloc_hook;
  __memalign_hook = old_memalign_hook;
}

static void
failmalloc_init (void)
{
  char *val;

  /* Obtain the probability from the environment.  */
  val = getenv ("FAILMALLOC_PROBABILITY");
  if (val)
    {
      double tmp;

      tmp = strtod (val, 0);
      if (tmp >= 0.0 && tmp <= 1.0)
        failure_probability = tmp;
    }

  /* Obtain the interval from the environment.  */
  val = getenv ("FAILMALLOC_INTERVAL");
  if (val)
    {
      unsigned long tmp;

      tmp = strtoul (val, 0, 0);
      if (tmp != ULONG_MAX)
        failure_interval = tmp;
    }
  
  /* Obtain the maximum number of failures from the environment.  */
  val = getenv ("FAILMALLOC_TIMES");
  if (val)
    {
      long tmp;

      tmp = strtol (val, 0, 0);
      if (tmp >= 0 && tmp != LONG_MAX)
        max_failures = tmp;
    }

  /* Obtain the maximum size of free space from the environment.  */
  val = getenv ("FAILMALLOC_SPACE");
  if (val)
    {
      unsigned long tmp;

      tmp = strtoul (val, 0, 0);
      if (tmp != LONG_MAX)
        max_space = tmp;
    }

  /* Initialize the random seed with something.  */
  srand (getpid () * 32452843 + time (NULL) * 49979687);

  failmalloc_install ();
}

void (*__malloc_initialize_hook) (void) = failmalloc_init;

int main(int argc, char *argv[])
{

    char *p = (char*) malloc(12);
    free(p);
    p = (char*) malloc(12);
    printf("%p\n", p);
    //*(int*)p = 12;
    free(p);
    return 0;
}/* -- end of main  -- */
