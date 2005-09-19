/* -*- mode: c -*- */
/* $Id$ */

/* Copyright (C) 2005 Alexander Chernov <cher@ispras.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define NEED_CORR 1
#define NEED_INFO 0
#define NEED_TGZ  0
#include "checker.h"

int checker_main(int argc, char **argv)
{
  double team_ans, corr_ans, eps;
  unsigned char *s;
  int n, i = 0;
  unsigned char buf[32];

  if (!(s = getenv("EPS")))
    fatal_CF("environment variable EPS is not set");
  if (sscanf(s, "%lf%n", &eps, &n) != 1 || s[n])
    fatal_CF("cannot parse EPS value");
  if (eps <= 0.0)
    fatal_CF("EPS <= 0");
  if (eps >= 1)
    fatal_CF("EPS >= 1");

  while (1) {
    i++;
    snprintf(buf, sizeof(buf), "[%d]", i);
    if (checker_read_corr_double(buf, 0, &corr_ans) < 0) break;
    checker_read_team_double(buf, 1, &team_ans);
    if (!checker_eq_double(corr_ans, team_ans, eps))
      fatal_WA("Answers differ: %s: team: %.10g, corr: %.10g", buf, team_ans, corr_ans);
  }
  checker_team_eof();

  checker_OK();
}

/*
 * Local variables:
 *  compile-command: "gcc -Wall -O2 -s -I. -L. cmp_double_seq.c -o cmp_double_seq -lchecker -lm"
 *  c-font-lock-extra-types: ("\\sw+_t" "FILE")
 * End:
 */
