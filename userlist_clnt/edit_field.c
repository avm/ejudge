/* -*- mode: c; coding: koi8-r -*- */
/* $Id$ */

/* Copyright (C) 2002,2003 Alexander Chernov <cher@ispras.ru> */

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

#include "userlist_clnt/private.h"

int
userlist_clnt_edit_field(struct userlist_clnt *clnt,
                         int user_id,
                         int role,
                         int pers,
                         int field,
                         unsigned char const *value)
{
  struct userlist_pk_edit_field *out = 0;
  struct userlist_packet *in = 0;
  int out_size = 0, in_size = 0, r, value_len;

  if (!value) value = "";
  value_len = strlen(value);
  if (value_len > 255) return -ULS_ERR_INVALID_SIZE;
  out_size = sizeof(*out) + value_len;
  out = alloca(out_size);
  memset(out, 0, out_size);
  out->request_id = ULS_EDIT_FIELD;
  out->user_id = user_id;
  out->role = role;
  out->pers = pers;
  out->field = field;
  out->value_len = value_len;
  strcpy(out->data, value);
  if ((r = userlist_clnt_send_packet(clnt, out_size, out)) < 0) return r;
  if ((r = userlist_clnt_recv_packet(clnt, &in_size, (void*) &in)) < 0)
    return r;
  if (in_size != sizeof(*in)) {
    xfree(in);
    return -ULS_ERR_PROTOCOL;
  }
  r = in->id;
  xfree(in);
  return r;
}

/**
 * Local variables:
 *  compile-command: "make -C .."
 *  c-font-lock-extra-types: ("\\sw+_t" "FILE")
 * End:
 */
