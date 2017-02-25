/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gvb-opt.h
 * Author: Miha
 *
 * Created on September 10, 2016, 8:57 PM
 */

#ifndef GVB_OPT_H
#define GVB_OPT_H

#include <glib.h>

const gchar*
gvb_opt_get_clean_name(const gchar *opt_name, gboolean *is_long);

const gchar*
gvb_opt_find_long_name(guint32 num_entries, const GOptionEntry *entries, const gchar *opt_name);

#endif /* GVB_OPT_H */

