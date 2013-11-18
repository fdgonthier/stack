/*
 * IPC Processes related utilities
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Sander Vrijders <sander.vrijders@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/export.h>

#define RINA_PREFIX "ipcp-utils"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "common.h"
#include "ipcp-utils.h"

struct name * name_create_gfp(gfp_t flags)
{ return rkzalloc(sizeof(struct name), flags); }
EXPORT_SYMBOL(name_create_gfp);

struct name * name_create(void)
{ return name_create_gfp(GFP_KERNEL); }
EXPORT_SYMBOL(name_create);

/*
 * NOTE:
 *
 * No needs to export the following string_* symbols for the time being. They
 * will be grouped here and moved into their own placeholder later on (as well
 * as all the "common" utilities). Lot of them should even be dropped ...
 *
 *   Francesco
 */
static int string_dup_gfp(gfp_t            flags,
                          const string_t * src,
                          string_t **      dst)
{
        if (!dst) {
                LOG_ERR("Destination string is NULL, cannot copy");
                return -1;
        }

        /*
         * An empty source is allowed (ref. the chain of calls) and it must
         * provoke no consequeunces
         */
        if (src) {
                *dst = kstrdup(src, flags);
                if (!*dst) {
                        LOG_ERR("Cannot duplicate source string "
                                "in kernel-space");
                        return -1;
                }
        } else {
                LOG_DBG("Duplicating a NULL source string ...");
                *dst = NULL;
        }

        return 0;
}

static int string_dup(const string_t * src, string_t ** dst)
{ return string_dup_gfp(GFP_KERNEL, src, dst); }

static int string_cmp(const string_t * a, const string_t * b)
{ return strcmp(a, b); }

/* FIXME: Should we assert here ? */
static int string_len(const string_t * s)
{ return strlen(s); }

/* FIXME: This thing is bogus and has to be fixed properly */
#ifdef CONFIG_RINA_DEBUG
static bool name_is_initialized(struct name * dst)
{
        ASSERT(dst);

        if (!dst->process_name     &&
            !dst->process_instance &&
            !dst->entity_name      &&
            !dst->entity_instance)
                return true;

        return false;
}
#else
static bool name_is_initialized(struct name * dst)
{
        ASSERT(dst);

        return true;
}
#endif

struct name * name_init_gfp(gfp_t            flags,
                            struct name *    dst,
                            const string_t * process_name,
                            const string_t * process_instance,
                            const string_t * entity_name,
                            const string_t * entity_instance)
{
        if (!dst)
                return NULL;

        /* Clean up the destination, leftovers might be there ... */
        name_fini(dst);

        ASSERT(name_is_initialized(dst));

        /* Boolean shortcuits ... */
        if (string_dup_gfp(flags, process_name,     &dst->process_name)     ||
            string_dup_gfp(flags, process_instance, &dst->process_instance) ||
            string_dup_gfp(flags, entity_name,      &dst->entity_name)      ||
            string_dup_gfp(flags, entity_instance,  &dst->entity_instance)) {
                name_fini(dst);
                return NULL;
        }

        return dst;
}
EXPORT_SYMBOL(name_init_gfp);

struct name * name_init(struct name *    dst,
                        const string_t * process_name,
                        const string_t * process_instance,
                        const string_t * entity_name,
                        const string_t * entity_instance)
{
        return name_init_gfp(GFP_KERNEL,
                             dst,
                             process_name,
                             process_instance,
                             entity_name,
                             entity_instance);
}
EXPORT_SYMBOL(name_init);

void name_fini(struct name * n)
{
        ASSERT(n);

        if (n->process_name) {
                rkfree(n->process_name);
                n->process_name = NULL;
        }
        if (n->process_instance) {
                rkfree(n->process_instance);
                n->process_instance = NULL;
        }
        if (n->entity_name) {
                rkfree(n->entity_name);
                n->entity_name = NULL;
        }
        if (n->entity_instance) {
                rkfree(n->entity_instance);
                n->entity_instance = NULL;
        }

        LOG_DBG("Name at %pK finalized successfully", n);
}
EXPORT_SYMBOL(name_fini);

void name_destroy(struct name * ptr)
{
        ASSERT(ptr);

        name_fini(ptr);

        ASSERT(name_is_initialized(ptr));

        rkfree(ptr);

        LOG_DBG("Name at %pK destroyed successfully", ptr);
}
EXPORT_SYMBOL(name_destroy);

int name_cpy(const struct name * src, struct name * dst)
{
        if (!src || !dst)
                return -1;

        LOG_DBG("Copying name %pK into %pK", src, dst);

        name_fini(dst);

        ASSERT(name_is_initialized(dst));

        /* We rely on short-boolean evaluation ... :-) */
        if (string_dup(src->process_name,     &dst->process_name)     ||
            string_dup(src->process_instance, &dst->process_instance) ||
            string_dup(src->entity_name,      &dst->entity_name)      ||
            string_dup(src->entity_instance,  &dst->entity_instance)) {
                name_fini(dst);
                return -1;
        }

        LOG_DBG("Name %pK copied successfully into %pK", src, dst);

        return 0;
}
EXPORT_SYMBOL(name_cpy);

struct name * name_dup(const struct name * src)
{
        struct name * tmp;

        if (!src)
                return NULL;

        tmp = name_create();
        if (!tmp)
                return NULL;
        if (name_cpy(src, tmp)) {
                name_destroy(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(name_dup);

#define NAME_CMP_FIELD(X, Y, FIELD)                                     \
        ((X->FIELD && Y->FIELD) ? string_cmp(X->FIELD, Y->FIELD) :      \
         ((!X->FIELD && !Y->FIELD) ? 0 : -1))

bool is_name_ok(const struct name * n)
{ return (!n && n->process_name); }

static int name_is_equal_internal(const struct name * a,
                                  const struct name * b)
{
        if (a == b)
                return 0;
        if (!a || !b)
                return -1;

        ASSERT(a != b);
        ASSERT(a != NULL);
        ASSERT(b != NULL);

        /* Now compare field by field */
        if (NAME_CMP_FIELD(a, b, process_name))
                return -1;
        if (NAME_CMP_FIELD(a, b, process_instance))
                return -1;
        if (NAME_CMP_FIELD(a, b, entity_name))
                return -1;
        if (NAME_CMP_FIELD(a, b, entity_instance))
                return -1;

        return 0;
}

bool name_is_equal(const struct name * a, const struct name * b)
{ return !name_is_equal_internal(a, b) ? true : false; }
EXPORT_SYMBOL(name_is_equal);

#define DELIMITER "/"

char * name_tostring_gfp(gfp_t               flags,
                         const struct name * n)
{
        char *       tmp;
        size_t       size;
        const char * none     = "<NONE>";
        size_t       none_len = strlen(none);

        if (!n)
                return NULL;

        size  = 0;

        size += (n->process_name                 ?
                 string_len(n->process_name)     : none_len);
        size += strlen(DELIMITER);

        size += (n->process_instance             ?
                 string_len(n->process_instance) : none_len);
        size += strlen(DELIMITER);

        size += (n->entity_name                  ?
                 string_len(n->entity_name)      : none_len);
        size += strlen(DELIMITER);

        size += (n->entity_instance              ?
                 string_len(n->entity_instance)  : none_len);
        size += strlen(DELIMITER);

        tmp = rkmalloc(size, flags);
        if (!tmp)
                return NULL;

        if (snprintf(tmp, size,
                     "%s%s%s%s%s%s%s",
                     (n->process_name     ? n->process_name     : none),
                     DELIMITER,
                     (n->process_instance ? n->process_instance : none),
                     DELIMITER,
                     (n->entity_name      ? n->entity_name      : none),
                     DELIMITER,
                     (n->entity_instance  ? n->entity_instance  : none)) !=
            size - 1) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(name_tostring_gfp);

char * name_tostring(const struct name * n)
{ return name_tostring_gfp(GFP_KERNEL, n); }
EXPORT_SYMBOL(name_tostring);

struct name * string_toname_gfp(gfp_t            flags,
                                const string_t * input)
{
        struct name * name;

        string_t *    tmp1   = NULL;
        string_t *    tmp_pn = NULL;
        string_t *    tmp_pi = NULL;
        string_t *    tmp_en = NULL;
        string_t *    tmp_ei = NULL;

        if (input) {
                string_t * tmp2;

                string_dup_gfp(flags, input, &tmp1);
                if (!tmp1) {
                        return NULL;
                }
                tmp2 = tmp1;

                tmp_pn = strsep(&tmp2, DELIMITER);
                tmp_pi = strsep(&tmp2, DELIMITER);
                tmp_en = strsep(&tmp2, DELIMITER);
                tmp_ei = strsep(&tmp2, DELIMITER);
        }

        name = name_create_gfp(flags);
        if (!name) {
                if (tmp1) rkfree(tmp1);
                return NULL;
        }

        if (!name_init_gfp(flags, name, tmp_pn, tmp_pi, tmp_en, tmp_ei)) {
                name_destroy(name);
                if (tmp1) rkfree(tmp1);
                return NULL;
        }

        if (tmp1) rkfree(tmp1);

        return name;
}
EXPORT_SYMBOL(string_toname_gfp);

struct name * string_toname(const string_t * input)
{ return string_toname_gfp(GFP_KERNEL, input); }
EXPORT_SYMBOL(string_toname);

static int string_dup_from_user(const string_t __user * src, string_t ** dst)
{
        ASSERT(dst);

        /*
         * An empty source is allowed (ref. the chain of calls) and it must
         * provoke no consequeunces
         */
        if (src) {
                *dst = strdup_from_user(src);
                if (!*dst) {
                        LOG_ERR("Cannot duplicate source string "
                                "from user-space");
                        return -1;
                }
        } else
                *dst = NULL;

        return 0;
}

int name_cpy_from_user(const struct name __user * src,
                       struct name *              dst)
{
        if (!src || !dst)
                return -1;

        name_fini(dst);

        ASSERT(name_is_initialized(dst));

        if (string_dup_from_user(src->process_name,
                                 &dst->process_name)) {
                LOG_ERR("Cannot dup process-name");
                name_fini(dst);
                return -1;
        }
        if (string_dup_from_user(src->process_instance,
                                 &dst->process_instance)) {
                LOG_ERR("Cannot dup process-instance");
                name_fini(dst);
                return -1;
        }
        if (string_dup_from_user(src->entity_name,
                                 &dst->entity_name)) {
                LOG_ERR("Cannot dup entity-name");
                name_fini(dst);
                return -1;
        }
        if (string_dup_from_user(src->entity_instance,
                                 &dst->entity_instance)) {
                LOG_ERR("Cannot dup entity-instance");
                name_fini(dst);
                return -1;
        }

        return 0;
}

struct name * name_dup_from_user(const struct name __user * src)
{
        struct name * tmp;

        LOG_DBG("Name duplication (from user) in progress");

        if (!src)
                return NULL;

        tmp = name_create();
        if (!tmp)
                return NULL;

        if (name_cpy_from_user(src, tmp)) {
                name_destroy(tmp);
                return NULL;
        }

        LOG_DBG("Name duplication (from user) completed successfully");

        return tmp;
}

struct ipcp_config * ipcp_config_create(void)
{
        struct ipcp_config * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->entry = NULL;
        INIT_LIST_HEAD(&tmp->next);

        return tmp;
}

int ipcp_config_destroy(struct ipcp_config * cfg)
{
        if (!cfg)
                return -1;

        if (cfg->entry)
                if (cfg->entry->name) rkfree(cfg->entry->name);
        if (cfg->entry->value) rkfree(cfg->entry->value);
        rkfree(cfg->entry);

        rkfree(cfg);

        return 0;
}

struct ipcp_config *
ipcp_config_dup_from_user(const struct ipcp_config __user * cfg)
{
        LOG_MISSING;

        return NULL;
}

struct connection * connection_create(void)
{
        struct connection * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        return tmp;
}

struct connection *
connection_dup_from_user(const struct connection __user * conn)
{
        struct connection * tmp = rkmalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        if (copy_from_user(tmp, conn, sizeof(*tmp)))
                return NULL;

        return tmp;
}

int connection_destroy(struct connection * conn)
{
        if (!conn)
                return -1;
        rkfree(conn);
        return 0;
}

struct flow_spec * flow_spec_dup(const struct flow_spec * fspec)
{
        struct flow_spec * tmp;

        if (!fspec)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp) {
                return NULL;
        }

        /* FIXME: Are these field by field copy really needed ? */
        /* FIXME: Please use proper indentation */
        tmp->average_bandwidth           = fspec->average_bandwidth;
        tmp->average_sdu_bandwidth       = fspec->average_sdu_bandwidth;
        tmp->delay                       = fspec->delay;
        tmp->jitter                      = fspec->jitter;
        tmp->max_allowable_gap           = fspec->max_allowable_gap;
        tmp->max_sdu_size                = fspec->max_sdu_size;
        tmp->ordered_delivery            = fspec->ordered_delivery;
        tmp->partial_delivery            = fspec->partial_delivery;
        tmp->peak_bandwidth_duration     = fspec->peak_bandwidth_duration;
        tmp->peak_sdu_bandwidth_duration = fspec->peak_sdu_bandwidth_duration;
        tmp->undetected_bit_error_rate   = fspec->undetected_bit_error_rate;

        return tmp;
}
EXPORT_SYMBOL(flow_spec_dup);

struct dif_config * dif_config_create(void)
{
        struct dif_config * tmp;

        tmp = rkzalloc(sizeof(struct dif_config), GFP_KERNEL);
        if (!tmp) {
                LOG_DBG("Could not create new dif_config");
                return NULL;
        }

        INIT_LIST_HEAD(&(tmp->ipcp_config_entries));
        return tmp;

}
EXPORT_SYMBOL(dif_config_create);

int dif_config_destroy(struct dif_config * dif_config)
{
        struct ipcp_config * pos, * nxt;

        if (!dif_config)
                return -1;

        list_for_each_entry_safe(pos, nxt,
                                 &dif_config->ipcp_config_entries,
                                 next) {
                list_del(&pos->next);
                ipcp_config_destroy(pos);
        }

        if (dif_config->data_transfer_constants)
                rkfree(dif_config->data_transfer_constants);
        rkfree(dif_config);

        return 0;
}
EXPORT_SYMBOL(dif_config_destroy);

int dif_info_destroy(struct dif_info * dif_info)
{
        if (dif_info) {
                if (dif_info->dif_name) {
                        name_destroy(dif_info->dif_name);
                }

                if (dif_info->configuration) {
                        if (dif_config_destroy(dif_info->configuration))
                                return -1;
                }

                rkfree(dif_info->type);
                rkfree(dif_info);
        }

        return 0;
}
EXPORT_SYMBOL(dif_info_destroy);
