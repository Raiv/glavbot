#include "gvb-opt.h"
#include "gvb-error.h"
#include <string.h>
#include <errno.h>

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
        if((strlen(clean_name)==1 && clean_name[0]==entries[idx].short_name)
            || (g_strcmp0(clean_name, entries[idx].long_name)==0))
        {
            return entries[idx].long_name;
        }
    }
    return NULL;
}

gboolean
gvb_opt_str_to_num(const char *value, guint32 *value_num, GError **error)
{
    if(!value || !value_num) {
        g_set_error(error, GVB_ERROR, GVB_ERROR_INVALID_ARG
                , "arg is null: [%p,%p]"
                , value, value_num
                );
        return FALSE;
    }
    
    gchar *endptr = NULL;
    *value_num = (guint32)g_ascii_strtoull(value, &endptr, 10);
    if(errno<0 || endptr==value) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE
                , "string conversion fails [%s] %s"
                , value, strerror(errno)
                );
        return FALSE;
    }
    
    return TRUE;
}