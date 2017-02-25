/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "gvb-opt.h"
#include <string.h>

const gchar*
gvb_opt_get_clean_name(const gchar *opt_name, gboolean *is_long)
{
    gint idx;
    const gchar *clean_name = opt_name;
    for(idx=0; idx<strlen(opt_name) && '-'==opt_name[idx]; idx++) {
        clean_name++;
    }
    if(is_long) {
        *is_long = (idx>1);
    }
    return clean_name;
}
    
const gchar*
gvb_opt_find_long_name(guint32 num_entries, const GOptionEntry *entries, const gchar *opt_name)
{
    gint idx;
    gboolean is_long;
    const gchar *clean_name;
    
    clean_name = gvb_opt_get_clean_name(opt_name, &is_long);
    
    // long option with '--'
    if(is_long) {
        return clean_name;
    }
    // short option with '-'
    g_assert_true(strlen(clean_name)==1);
    
    for(idx=0; idx<num_entries; idx++) {
        if(strlen(clean_name)==1 && clean_name[0]==entries[idx].short_name
            || g_strcmp0(clean_name, entries[idx].long_name)==0)
        {
            return entries[idx].long_name;
        }
    }
}