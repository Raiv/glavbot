#ifndef GVB_OPT_H
#define GVB_OPT_H

#include <glib.h>

const gchar*
gvb_opt_get_clean_name(const gchar *opt_name, gboolean *is_long);

const gchar*
gvb_opt_find_long_name(guint32 num_entries, const GOptionEntry *entries, const gchar *opt_name);

gboolean
gvb_opt_str_to_num(const char *value, guint32 *value_num, GError **error);

#endif /* GVB_OPT_H */

