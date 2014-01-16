/*
 * RNL utilities
 *
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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
#include <net/netlink.h>
#include <net/genetlink.h>
#include <linux/rculist.h>

#define RINA_PREFIX "rnl-utils"

#include "logs.h"
#include "common.h"
#include "debug.h"
#include "ipcp-utils.h"
#include "utils.h"
#include "rnl.h"
#include "rnl-utils.h"

/*
 * FIXME: I suppose these functions are internal (at least for the time being)
 *        therefore have been "statified" to avoid polluting the common name
 *        space (while allowing the compiler to present us "unused functions"
 *        warning messages (which would be unpossible if declared not-static)
 *
 * Francesco
 */

/*
 * FIXME: If some of them remain 'static', parameters checking has to be
 *        trasformed into ASSERT() calls (since msg is checked in the caller)
 *
 * NOTE: that functionalities exported to "shims" should prevent "evoking"
 *       ASSERT() here ...
 *
 * Francesco
 */

/*
 * FIXME: Destination is usually at the end of the prototype, not at the
 * beginning (e.g. msg and name)
 */

#define BUILD_STRERROR(X)                                       \
        "Netlink message does not contain " X ", bailing out"

#define BUILD_STRERROR_BY_MTYPE(X)                      \
        "Could not parse Netlink message of type " X

extern struct genl_family rnl_nl_family;

char * nla_get_string(struct nlattr * nla)
{ return (char *) nla_data(nla); }

static struct rnl_ipcm_alloc_flow_req_msg_attrs *
rnl_ipcm_alloc_flow_req_msg_attrs_create(void)
{
        struct rnl_ipcm_alloc_flow_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->source = name_create();
        if (!tmp->source) {
                rkfree(tmp);
                return NULL;
        }

        tmp->dest = name_create();
        if (!tmp->dest) {
                name_destroy(tmp->source);
                rkfree(tmp);
                return NULL;
        }

        tmp->dif_name = name_create();
        if (!tmp->dif_name) {
                name_destroy(tmp->dest);
                name_destroy(tmp->source);
                rkfree(tmp);
                return NULL;
        }

        tmp->fspec = rkzalloc(sizeof(struct flow_spec), GFP_KERNEL);
        if (!tmp->fspec) {
                name_destroy(tmp->dif_name);
                name_destroy(tmp->dest);
                name_destroy(tmp->source);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_alloc_flow_resp_msg_attrs *
rnl_alloc_flow_resp_msg_attrs_create(void)
{
        struct rnl_alloc_flow_resp_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcm_dealloc_flow_req_msg_attrs *
rnl_ipcm_dealloc_flow_req_msg_attrs_create(void)
{
        struct rnl_ipcm_dealloc_flow_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcm_assign_to_dif_req_msg_attrs *
rnl_ipcm_assign_to_dif_req_msg_attrs_create(void)
{
        struct rnl_ipcm_assign_to_dif_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->dif_info = rkzalloc(sizeof(struct dif_info), GFP_KERNEL);
        if (!tmp->dif_info) {
                rkfree(tmp);
                return NULL;
        }

        tmp->dif_info->dif_name = name_create();
        if (!tmp->dif_info->dif_name) {
                rkfree(tmp->dif_info);
                rkfree(tmp);
                return NULL;
        }

        tmp->dif_info->configuration = dif_config_create();
        if (!tmp->dif_info->configuration) {
                name_destroy(tmp->dif_info->dif_name);
                rkfree(tmp->dif_info);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcm_update_dif_config_req_msg_attrs *
rnl_ipcm_update_dif_config_req_msg_attrs_create(void)
{
        struct rnl_ipcm_update_dif_config_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->dif_config = dif_config_create();
        if (!tmp->dif_config) {
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcm_reg_app_req_msg_attrs *
rnl_ipcm_reg_app_req_msg_attrs_create(void)
{
        struct rnl_ipcm_reg_app_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        tmp->app_name = name_create();
        if (!tmp->app_name) {
                rkfree(tmp);
                return NULL;
        }

        tmp->dif_name = name_create();
        if (!tmp->dif_name) {
                name_destroy(tmp->app_name);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}

static struct rnl_ipcp_conn_create_req_msg_attrs *
rnl_ipcp_conn_create_req_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_create_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcp_conn_create_arrived_msg_attrs *
rnl_ipcp_conn_create_arrived_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_create_arrived_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcp_conn_update_req_msg_attrs *
rnl_ipcp_conn_update_req_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_update_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_ipcp_conn_destroy_req_msg_attrs *
rnl_ipcp_conn_destroy_req_msg_attrs_create(void)
{
        struct rnl_ipcp_conn_destroy_req_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        return tmp;
}

static struct rnl_rmt_mod_pfte_msg_attrs *
rnl_rmt_mod_pfte_msg_attrs_create(void)
{
        struct rnl_rmt_mod_pfte_msg_attrs * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if  (!tmp)
                return NULL;

        INIT_LIST_HEAD(&tmp->pft_entries);

        return tmp;
}

struct rnl_msg * rnl_msg_create(enum rnl_msg_attr_type type)
{
        struct rnl_msg * tmp;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->attr_type = type;

        switch (tmp->attr_type) {
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_REQUEST:
                tmp->attrs =
                        rnl_ipcm_alloc_flow_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_RESPONSE:
                tmp->attrs =
                        rnl_alloc_flow_resp_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_DEALLOCATE_FLOW_REQUEST:
                tmp->attrs =
                        rnl_ipcm_dealloc_flow_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_ASSIGN_TO_DIF_REQUEST:
                tmp->attrs =
                        rnl_ipcm_assign_to_dif_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_UPDATE_DIF_CONFIG_REQUEST:
                tmp->attrs =
                        rnl_ipcm_update_dif_config_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_REG_UNREG_REQUEST:
                tmp->attrs =
                        rnl_ipcm_reg_app_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_REQUEST:
                tmp->attrs =
                        rnl_ipcp_conn_create_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_ARRIVED:
                tmp->attrs =
                        rnl_ipcp_conn_create_arrived_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_UPDATE_REQUEST:
                tmp->attrs =
                        rnl_ipcp_conn_update_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_CONN_DESTROY_REQUEST:
                tmp->attrs =
                        rnl_ipcp_conn_destroy_req_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_RMT_PFTE_MODIFY_REQUEST:
                tmp->attrs =
                        rnl_rmt_mod_pfte_msg_attrs_create();
                if (!tmp->attrs) {
                        rkfree(tmp);
                        return NULL;
                }
                break;
        case RNL_MSG_ATTRS_RMT_PFT_DUMP_REQUEST:
                        tmp->attrs = NULL;
        default:
                LOG_ERR("Unknown attributes type %d", tmp->attr_type);
                rkfree(tmp);
                return NULL;
        }

        return tmp;
}
EXPORT_SYMBOL(rnl_msg_create);

static int
rnl_ipcm_alloc_flow_req_msg_attrs_destroy(struct rnl_ipcm_alloc_flow_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->source)   name_destroy(attrs->source);
        if (attrs->dest)     name_destroy(attrs->dest);
        if (attrs->dif_name) name_destroy(attrs->dif_name);
        if (attrs->fspec)    rkfree(attrs->fspec);
        rkfree(attrs);

        return 0;
}

static int
rnl_alloc_flow_resp_msg_attrs_destroy(struct rnl_alloc_flow_resp_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_dealloc_flow_req_msg_attrs_destroy(struct rnl_ipcm_dealloc_flow_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_assign_to_dif_req_msg_attrs_destroy(struct rnl_ipcm_assign_to_dif_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->dif_info) {
                if (attrs->dif_info->dif_name)
                        name_destroy(attrs->dif_info->dif_name);
                if (attrs->dif_info->configuration)
                        dif_config_destroy(attrs->dif_info->configuration);
                rkfree(attrs->dif_info);
        }

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_update_dif_config_req_msg_attrs_destroy(struct rnl_ipcm_update_dif_config_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->dif_config) {
                dif_config_destroy(attrs->dif_config);
        }

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcm_reg_app_req_msg_attrs_destroy(struct rnl_ipcm_reg_app_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        if (attrs->app_name) name_destroy(attrs->app_name);
        if (attrs->dif_name) name_destroy(attrs->dif_name);

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_create_req_msg_attrs_destroy(struct rnl_ipcp_conn_create_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_create_arrived_msg_attrs_destroy(struct rnl_ipcp_conn_create_arrived_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_update_req_msg_attrs_destroy(struct rnl_ipcp_conn_update_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_ipcp_conn_destroy_req_msg_attrs_destroy(struct rnl_ipcp_conn_destroy_req_msg_attrs * attrs)
{
        if (!attrs)
                return -1;

        rkfree(attrs);
        return 0;
}

static int
rnl_rmt_mod_pfte_msg_attrs_destroy(struct rnl_rmt_mod_pfte_msg_attrs * attrs)
{
        struct pdu_ft_entry * pos, * nxt;

        if (!attrs)
                return -1;

        list_for_each_entry_safe(pos, nxt,
                                 &attrs->pft_entries,
                                 next) {

                if (pos->ports) rkfree(pos->ports);

                list_del(&pos->next);
                rkfree(pos);
        }
        LOG_DBG("rnl_rmt_mod_pfte_msg_attrs destroy correctly");
        rkfree(attrs);
        return 0;
}

int rnl_msg_destroy(struct rnl_msg * msg)
{
        if (!msg)
                return -1;

        switch(msg->attr_type) {
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_REQUEST:
                rnl_ipcm_alloc_flow_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_ALLOCATE_FLOW_RESPONSE:
                rnl_alloc_flow_resp_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_DEALLOCATE_FLOW_REQUEST:
                rnl_ipcm_dealloc_flow_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_ASSIGN_TO_DIF_REQUEST:
                rnl_ipcm_assign_to_dif_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_UPDATE_DIF_CONFIG_REQUEST:
                rnl_ipcm_update_dif_config_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_REG_UNREG_REQUEST:
                rnl_ipcm_reg_app_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_REQUEST:
                rnl_ipcp_conn_create_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_CREATE_ARRIVED:
                rnl_ipcp_conn_create_arrived_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_UPDATE_REQUEST:
                rnl_ipcp_conn_update_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_CONN_DESTROY_REQUEST:
                rnl_ipcp_conn_destroy_req_msg_attrs_destroy(msg->attrs);
                break;
        case RNL_MSG_ATTRS_RMT_PFTE_MODIFY_REQUEST:
                rnl_rmt_mod_pfte_msg_attrs_destroy(msg->attrs);
                break;
        default:
                break;
        }

        rkfree(msg);

        return 0;
}
EXPORT_SYMBOL(rnl_msg_destroy);

static int parse_flow_spec(struct nlattr * fspec_attr,
                           struct flow_spec * fspec_struct)
{
        struct nla_policy attr_policy[FSPEC_ATTR_MAX + 1];
        struct nlattr *   attrs[FSPEC_ATTR_MAX + 1];

        attr_policy[FSPEC_ATTR_AVG_BWITH].type               = NLA_U32;
        attr_policy[FSPEC_ATTR_AVG_BWITH].len                = 4;
        attr_policy[FSPEC_ATTR_AVG_SDU_BWITH].type           = NLA_U32;
        attr_policy[FSPEC_ATTR_AVG_SDU_BWITH].len            = 4;
        attr_policy[FSPEC_ATTR_DELAY].type                   = NLA_U32;
        attr_policy[FSPEC_ATTR_DELAY].len                    = 4;
        attr_policy[FSPEC_ATTR_JITTER].type                  = NLA_U32;
        attr_policy[FSPEC_ATTR_JITTER].len                   = 4;
        attr_policy[FSPEC_ATTR_MAX_GAP].type                 = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_GAP].len                  = 4;
        attr_policy[FSPEC_ATTR_MAX_SDU_SIZE].type            = NLA_U32;
        attr_policy[FSPEC_ATTR_MAX_SDU_SIZE].len             = 4;
        attr_policy[FSPEC_ATTR_IN_ORD_DELIVERY].type         = NLA_FLAG;
        attr_policy[FSPEC_ATTR_IN_ORD_DELIVERY].len          = 0;
        attr_policy[FSPEC_ATTR_PART_DELIVERY].type           = NLA_FLAG;
        attr_policy[FSPEC_ATTR_PART_DELIVERY].len            = 0;
        attr_policy[FSPEC_ATTR_PEAK_BWITH_DURATION].type     = NLA_U32;
        attr_policy[FSPEC_ATTR_PEAK_BWITH_DURATION].len      = 4;
        attr_policy[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION].type = NLA_U32;
        attr_policy[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION].len  = 4;
        attr_policy[FSPEC_ATTR_UNDETECTED_BER].type          = NLA_U32;
        attr_policy[FSPEC_ATTR_UNDETECTED_BER].len           = 4;

        if (nla_parse_nested(attrs,
                             FSPEC_ATTR_MAX,
                             fspec_attr,
                             attr_policy) < 0)
                return -1;

        if (attrs[FSPEC_ATTR_AVG_BWITH])
                /* FIXME: min = max in uint_range */
                fspec_struct->average_bandwidth =
                        nla_get_u32(attrs[FSPEC_ATTR_AVG_BWITH]);

        if (attrs[FSPEC_ATTR_AVG_SDU_BWITH])
                fspec_struct->average_sdu_bandwidth =
                        nla_get_u32(attrs[FSPEC_ATTR_AVG_SDU_BWITH]);

        if (attrs[FSPEC_ATTR_PEAK_BWITH_DURATION])
                fspec_struct->peak_bandwidth_duration =
                        nla_get_u32(attrs[FSPEC_ATTR_PEAK_BWITH_DURATION]);

        if (attrs[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION])
                fspec_struct->peak_sdu_bandwidth_duration =
                        nla_get_u32(attrs[FSPEC_ATTR_PEAK_SDU_BWITH_DURATION]);

        if (attrs[FSPEC_ATTR_UNDETECTED_BER])
                fspec_struct->undetected_bit_error_rate = \
                        nla_get_u32(attrs[FSPEC_ATTR_UNDETECTED_BER]);

        if (attrs[FSPEC_ATTR_UNDETECTED_BER])
                fspec_struct->undetected_bit_error_rate = \
                        nla_get_u32(attrs[FSPEC_ATTR_UNDETECTED_BER]);

        if (attrs[FSPEC_ATTR_PART_DELIVERY])
                fspec_struct->partial_delivery = \
                        nla_get_flag(attrs[FSPEC_ATTR_PART_DELIVERY]);

        if (attrs[FSPEC_ATTR_IN_ORD_DELIVERY])
                fspec_struct->ordered_delivery = \
                        nla_get_flag(attrs[FSPEC_ATTR_IN_ORD_DELIVERY]);

        if (attrs[FSPEC_ATTR_MAX_GAP])
                fspec_struct->max_allowable_gap = \
                        (int) nla_get_u32(attrs[FSPEC_ATTR_MAX_GAP]);

        if (attrs[FSPEC_ATTR_DELAY])
                fspec_struct->delay = \
                        nla_get_u32(attrs[FSPEC_ATTR_DELAY]);

        if (attrs[FSPEC_ATTR_MAX_SDU_SIZE])
                fspec_struct->max_sdu_size = \
                        nla_get_u32(attrs[FSPEC_ATTR_MAX_SDU_SIZE]);

        return 0;
}

static int parse_pdu_fte_port_list_entries(struct nlattr *       nested_attr,
                                           struct pdu_ft_entry * entry)
{
        int                           rem        = 0;
        int                           i          = 0;
        struct nlattr *               nla;

        if (!nested_attr) {
                LOG_ERR("Bogus nested attribute (ports) passed, bailing out");
                return -1;
        }

        if (!entry) {
                LOG_ERR("Bogus PFT entry passed, bailing out");
                return -1;
        }

        entry->ports_size = nla_len(nested_attr);
        entry->ports = rkzalloc(entry->ports_size, GFP_KERNEL);
        LOG_DBG("Allocated %d bytes in %pk", entry->ports_size, entry->ports);

        if (!entry->ports) {
                LOG_ERR("Could not allocate memory for ports");
                return -1;
        }

        for (nla = (struct nlattr*) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &rem)) {
                entry->ports[i] = nla_get_u32(nla);
                i++;
        }

        entry->ports_size = (size_t) i;

        if (rem)
                LOG_WARN("Missing bits to pars");

        return 0;

}

static int parse_pdu_fte_list_entry(struct nlattr *       attr,
                                    struct pdu_ft_entry * pfte_struct)
{
        struct nla_policy             attr_policy[PFTELE_ATTR_MAX + 1];
        struct nlattr *               attrs[PFTELE_ATTR_MAX + 1];

        attr_policy[PFTELE_ATTR_ADDRESS].type         = NLA_U32;
        attr_policy[PFTELE_ATTR_ADDRESS].len          = 4;
        attr_policy[PFTELE_ATTR_QOSID].type           = NLA_U32;
        attr_policy[PFTELE_ATTR_QOSID].len            = 4;
        attr_policy[PFTELE_ATTR_PORTIDS].type          = NLA_NESTED;
        attr_policy[PFTELE_ATTR_PORTIDS].len           = 0;

        if (nla_parse_nested(attrs,
                             PFTELE_ATTR_MAX,
                             attr,
                             attr_policy) < 0)
                return -1;

        if (attrs[PFTELE_ATTR_ADDRESS])
                pfte_struct->destination =
                        nla_get_u32(attrs[PFTELE_ATTR_ADDRESS]);

        if (attrs[PFTELE_ATTR_QOSID])
                pfte_struct->qos_id =
                        nla_get_u32(attrs[PFTELE_ATTR_QOSID]);

        if (attrs[PFTELE_ATTR_PORTIDS]) {
                if (parse_pdu_fte_port_list_entries(attrs[PFTELE_ATTR_PORTIDS],
                                                    pfte_struct))
                        return -1;

        }

        return 0;
}

static int parse_app_name_info(struct nlattr * name_attr,
                               struct name *   name_struct)
{
        struct nla_policy attr_policy[APNI_ATTR_MAX + 1];
        struct nlattr *   attrs[APNI_ATTR_MAX + 1];
        string_t *        process_name;
        string_t *        process_instance;
        string_t *        entity_name;
        string_t *        entity_instance;

        if (!name_attr || !name_struct) {
                LOG_ERR("Bogus input parameters, cannot parse name app info");
                return -1;
        }

        attr_policy[APNI_ATTR_PROCESS_NAME].type     = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_NAME].len      = 0;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].type = NLA_STRING;
        attr_policy[APNI_ATTR_PROCESS_INSTANCE].len  = 0;
        attr_policy[APNI_ATTR_ENTITY_NAME].type      = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_NAME].len       = 0;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].type  = NLA_STRING;
        attr_policy[APNI_ATTR_ENTITY_INSTANCE].len   = 0;

        if (nla_parse_nested(attrs, APNI_ATTR_MAX, name_attr, attr_policy) < 0)
                return -1;


        if (attrs[APNI_ATTR_PROCESS_NAME])
                process_name =
                        nla_get_string(attrs[APNI_ATTR_PROCESS_NAME]);
        else
                process_name = NULL;

        if (attrs[APNI_ATTR_PROCESS_INSTANCE])
                process_instance =
                        nla_get_string(attrs[APNI_ATTR_PROCESS_INSTANCE]);
        else
                process_instance = NULL;

        if (attrs[APNI_ATTR_ENTITY_NAME])
                entity_name =
                        nla_get_string(attrs[APNI_ATTR_ENTITY_NAME]);
        else
                entity_name = NULL;

        if (attrs[APNI_ATTR_ENTITY_INSTANCE])
                entity_instance =
                        nla_get_string(attrs[APNI_ATTR_ENTITY_INSTANCE]);
        else
                entity_instance = NULL;

        if (!name_init_from(name_struct,
                            process_name, process_instance,
                            entity_name,  entity_instance))
                return -1;

        return 0;
}

static int parse_ipcp_config_entry_value(struct nlattr *            name_attr,
                                         struct ipcp_config_entry * entry)
{
        struct nla_policy attr_policy[IPCP_CONFIG_ENTRY_ATTR_MAX + 1];
        struct nlattr *   attrs[IPCP_CONFIG_ENTRY_ATTR_MAX + 1];

        if (!name_attr) {
                LOG_ERR("Bogus attribute passed, bailing out");
                return -1;
        }

        if (!entry) {
                LOG_ERR("Bogus entry passed, bailing out");
                return -1;
        }

        attr_policy[IPCP_CONFIG_ENTRY_ATTR_NAME].type  = NLA_STRING;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_NAME].len   = 0;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_VALUE].type = NLA_STRING;
        attr_policy[IPCP_CONFIG_ENTRY_ATTR_VALUE].len  = 0;

        if (nla_parse_nested(attrs, IPCP_CONFIG_ENTRY_ATTR_MAX,
                             name_attr, attr_policy) < 0)
                return -1;

        if (attrs[IPCP_CONFIG_ENTRY_ATTR_NAME])
                entry->name = kstrdup(nla_get_string(attrs[IPCP_CONFIG_ENTRY_ATTR_NAME]), GFP_KERNEL);

        if (attrs[IPCP_CONFIG_ENTRY_ATTR_VALUE])
                entry->value = kstrdup(nla_get_string(attrs[IPCP_CONFIG_ENTRY_ATTR_VALUE]), GFP_KERNEL);

        return 0;
}

static int parse_list_of_ipcp_config_entries(struct nlattr *     nested_attr,
                                             struct dif_config * dif_config)
{
        struct nlattr *            nla;
        struct ipcp_config_entry * entry;
        struct ipcp_config *       config;
        int                        rem                   = 0;
        int                        entries_with_problems = 0;
        int                        total_entries         = 0;

        if (!nested_attr) {
                LOG_ERR("Bogus attribute passed, bailing out");
                return -1;
        }

        if (!dif_config) {
                LOG_ERR("Bogus dif_config passed, bailing out");
                return -1;
        }

        for (nla = (struct nlattr*) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &(rem))) {
                total_entries++;

                entry = rkzalloc(sizeof(*entry), GFP_KERNEL);
                if (!entry) {
                        entries_with_problems++;
                        continue;
                }

                if (parse_ipcp_config_entry_value(nla, entry) < 0) {
                        rkfree(entry);
                        entries_with_problems++;
                        continue;
                }

                config = ipcp_config_create();
                if (!config) {
                        rkfree(entry);
                        entries_with_problems++;
                        continue;
                }
                config->entry = entry;
                list_add(&config->next, &dif_config->ipcp_config_entries);
        }

        if (rem > 0) {
                LOG_WARN("Missing bits to parse");
        }

        if (entries_with_problems > 0)
                LOG_WARN("Problems parsing %d out of %d parameters",
                         entries_with_problems,
                         total_entries);

        return 0;
}

static int parse_dt_cons(struct nlattr *  attr,
                         struct dt_cons * dt_cons)
{
        struct nla_policy attr_policy[DTC_ATTR_MAX + 1];
        struct nlattr *   attrs[DTC_ATTR_MAX + 1];

        attr_policy[DTC_ATTR_QOS_ID].type        = NLA_U16;
        attr_policy[DTC_ATTR_QOS_ID].len         = 2;
        attr_policy[DTC_ATTR_PORT_ID].type       = NLA_U16;
        attr_policy[DTC_ATTR_PORT_ID].len        = 2;
        attr_policy[DTC_ATTR_CEP_ID].type        = NLA_U16;
        attr_policy[DTC_ATTR_CEP_ID].len         = 2;
        attr_policy[DTC_ATTR_SEQ_NUM].type       = NLA_U16;
        attr_policy[DTC_ATTR_SEQ_NUM].len        = 2;
        attr_policy[DTC_ATTR_ADDRESS].type       = NLA_U16;
        attr_policy[DTC_ATTR_ADDRESS].len        = 2;
        attr_policy[DTC_ATTR_LENGTH].type        = NLA_U16;
        attr_policy[DTC_ATTR_LENGTH].len         = 2;
        attr_policy[DTC_ATTR_MAX_PDU_SIZE].type  = NLA_U32;
        attr_policy[DTC_ATTR_MAX_PDU_SIZE].len   = 4;
        attr_policy[DTC_ATTR_MAX_PDU_LIFE].type  = NLA_U32;
        attr_policy[DTC_ATTR_MAX_PDU_LIFE].len   = 4;
        attr_policy[DTC_ATTR_DIF_INTEGRITY].type = NLA_FLAG;
        attr_policy[DTC_ATTR_DIF_INTEGRITY].len  = 0;

        if (nla_parse_nested(attrs, DTC_ATTR_MAX, attr, attr_policy) < 0)
                return -1;

        if (attrs[DTC_ATTR_QOS_ID])
                dt_cons->qos_id_length =
                        nla_get_u16(attrs[DTC_ATTR_QOS_ID]);

        if (attrs[DTC_ATTR_PORT_ID])
                dt_cons->port_id_length =
                        nla_get_u16(attrs[DTC_ATTR_PORT_ID]);

        if (attrs[DTC_ATTR_CEP_ID])
                dt_cons->cep_id_length =
                        nla_get_u16(attrs[DTC_ATTR_CEP_ID]);

        if (attrs[DTC_ATTR_SEQ_NUM])
                dt_cons->seq_num_length =
                        nla_get_u16(attrs[DTC_ATTR_SEQ_NUM]);

        if (attrs[DTC_ATTR_ADDRESS])
                dt_cons->address_length =
                        nla_get_u16(attrs[DTC_ATTR_ADDRESS]);

        if (attrs[DTC_ATTR_LENGTH])
                dt_cons->length_length =
                        nla_get_u16(attrs[DTC_ATTR_LENGTH]);

        if (attrs[DTC_ATTR_MAX_PDU_SIZE])
                dt_cons->max_pdu_size =
                        nla_get_u32(attrs[DTC_ATTR_MAX_PDU_SIZE]);

        if (attrs[DTC_ATTR_MAX_PDU_LIFE])
                dt_cons->max_pdu_life =
                        nla_get_u32(attrs[DTC_ATTR_MAX_PDU_LIFE]);

        if (attrs[DTC_ATTR_DIF_INTEGRITY])
                dt_cons->dif_integrity = true;

        return 0;
}

static int parse_dif_config(struct nlattr * dif_config_attr,
                            struct dif_config  * dif_config)
{
        struct nla_policy attr_policy[DCONF_ATTR_MAX + 1];
        struct nlattr *   attrs[DCONF_ATTR_MAX + 1];
        struct dt_cons *  dt_cons;

        attr_policy[DCONF_ATTR_IPCP_CONFIG_ENTRIES].type = NLA_NESTED;
        attr_policy[DCONF_ATTR_IPCP_CONFIG_ENTRIES].len = 0;
        attr_policy[DCONF_ATTR_DATA_TRANS_CONS].type    = NLA_NESTED;
        attr_policy[DCONF_ATTR_DATA_TRANS_CONS].len     = 0;
        attr_policy[DCONF_ATTR_ADDRESS].type            = NLA_U32;
        attr_policy[DCONF_ATTR_ADDRESS].len             = 4;
        attr_policy[DCONF_ATTR_QOS_CUBES].type          = NLA_NESTED;
        attr_policy[DCONF_ATTR_QOS_CUBES].len           = 0;

        if (nla_parse_nested(attrs,
                             DCONF_ATTR_MAX,
                             dif_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[DCONF_ATTR_IPCP_CONFIG_ENTRIES]) {
                if (parse_list_of_ipcp_config_entries(attrs[DCONF_ATTR_IPCP_CONFIG_ENTRIES],
                                                      dif_config) < 0)
                        goto parse_fail;
        }

        if (attrs[DCONF_ATTR_DATA_TRANS_CONS]) {
                dt_cons = rkzalloc(sizeof(struct dt_cons),GFP_KERNEL);
                if (!dt_cons)
                        goto parse_fail;

                dif_config->dt_cons = dt_cons;

                if (parse_dt_cons(attrs[DCONF_ATTR_DATA_TRANS_CONS],
                                  dif_config->dt_cons) < 0) {
                        rkfree(dif_config->dt_cons);
                        goto parse_fail;
                }
        }

        if (attrs[DCONF_ATTR_ADDRESS])
                dif_config->address = nla_get_u32(attrs[DCONF_ATTR_ADDRESS]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("dif config attributes"));
        return -1;
}

static int parse_dif_info(struct nlattr *    dif_config_attr,
                          struct dif_info  * dif_info)
{
        struct nla_policy attr_policy[DINFO_ATTR_MAX + 1];
        struct nlattr *   attrs[DINFO_ATTR_MAX + 1];

        attr_policy[DINFO_ATTR_DIF_TYPE].type = NLA_STRING;
        attr_policy[DINFO_ATTR_DIF_TYPE].len  = 0;
        attr_policy[DINFO_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[DINFO_ATTR_DIF_NAME].len  = 0;
        attr_policy[DINFO_ATTR_CONFIG].type   = NLA_NESTED;
        attr_policy[DINFO_ATTR_CONFIG].len    = 0;

        if (nla_parse_nested(attrs,
                             DINFO_ATTR_MAX,
                             dif_config_attr,
                             attr_policy) < 0)
                goto parse_fail;

        if (attrs[DINFO_ATTR_DIF_TYPE])
                dif_info->type =
                        kstrdup(nla_get_string(attrs[DINFO_ATTR_DIF_TYPE]),
                                GFP_KERNEL);

        if (parse_app_name_info(attrs[DINFO_ATTR_DIF_NAME],
                                dif_info->dif_name) < 0)
                goto parse_fail;

        if (attrs[DINFO_ATTR_CONFIG])
                if (parse_dif_config(attrs[DINFO_ATTR_CONFIG],
                                     dif_info->configuration) < 0)
                        goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("dif info attribute"));
        return -1;
}

static int parse_rib_object(struct nlattr     * rib_obj_attr,
                            struct rib_object * rib_obj_struct)
{
        struct nla_policy attr_policy[RIBO_ATTR_MAX + 1];
        struct nlattr *attrs[RIBO_ATTR_MAX + 1];

        attr_policy[RIBO_ATTR_OBJECT_CLASS].type    = NLA_U32;
        attr_policy[RIBO_ATTR_OBJECT_NAME].type     = NLA_STRING;
        attr_policy[RIBO_ATTR_OBJECT_INSTANCE].type = NLA_U32;

        if (nla_parse_nested(attrs,
                             RIBO_ATTR_MAX, rib_obj_attr, attr_policy) < 0)
                return -1;

        if (attrs[RIBO_ATTR_OBJECT_CLASS])
                rib_obj_struct->rib_obj_class =\
                        nla_get_u32(&rib_obj_attr[RIBO_ATTR_OBJECT_CLASS]);

        if (attrs[RIBO_ATTR_OBJECT_NAME])
                nla_strlcpy(rib_obj_struct->rib_obj_name,
                            attrs[RIBO_ATTR_OBJECT_NAME],
                            sizeof(attrs[RIBO_ATTR_OBJECT_NAME]));
        if (attrs[RIBO_ATTR_OBJECT_INSTANCE])
                rib_obj_struct->rib_obj_instance =\
                        nla_get_u32(&rib_obj_attr[RIBO_ATTR_OBJECT_INSTANCE]);
        return 0;
}

static int rnl_parse_generic_u32_param_msg(struct genl_info * info,
                                           uint_t *           param_var)
{

        struct nla_policy attr_policy[GOA_ATTR_MAX + 1];
        struct nlattr *   attrs[GOA_ATTR_MAX + 1];
        int               result;

        attr_policy[GOA_ATTR_ONE].type = NLA_U32;
        attr_policy[GOA_ATTR_ONE].len  = 4;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             GOA_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy",
                        result);
                goto parse_fail;
        }

        if (attrs[GOA_ATTR_ONE]) {
                * param_var = nla_get_u32(attrs[GOA_ATTR_ONE]);
        }

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("GENERIC_U_32_PARAM_MSG"));
        return -1;
}

static int
rnl_parse_ipcm_assign_to_dif_req_msg(struct genl_info * info,
                                     struct rnl_ipcm_assign_to_dif_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IATDR_ATTR_MAX + 1];
        struct nlattr *   attrs[IATDR_ATTR_MAX + 1];
        int               result;

        attr_policy[IATDR_ATTR_DIF_INFORMATION].type = NLA_NESTED;
        attr_policy[IATDR_ATTR_DIF_INFORMATION].len  = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IATDR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy",
                        result);
                goto parse_fail;
        }

        if (parse_dif_info(attrs[IATDR_ATTR_DIF_INFORMATION],
                           msg_attrs->dif_info) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_update_dif_config_req_msg
(struct genl_info * info,
struct rnl_ipcm_update_dif_config_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IUDCR_ATTR_MAX + 1];
        struct nlattr *attrs[IUDCR_ATTR_MAX + 1];
        int result;

        attr_policy[IUDCR_ATTR_DIF_CONFIGURATION].type = NLA_NESTED;
        attr_policy[IUDCR_ATTR_DIF_CONFIGURATION].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IUDCR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy",
                        result);
                goto parse_fail;
        }

        if (parse_dif_config(attrs[IUDCR_ATTR_DIF_CONFIGURATION],
                             msg_attrs->dif_config) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_ipcp_dif_reg_noti_msg(struct genl_info * info,
                                                struct rnl_ipcm_ipcp_dif_reg_noti_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDRN_ATTR_MAX + 1];
        struct nlattr *attrs[IDRN_ATTR_MAX + 1];
        int result;

        attr_policy[IDRN_ATTR_IPC_PROCESS_NAME].type = NLA_NESTED;
        attr_policy[IDRN_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IDRN_ATTR_REGISTRATION].type = NLA_FLAG;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IDRN_ATTR_MAX,
                             attr_policy);

        if (info->attrs[IDRN_ATTR_IPC_PROCESS_NAME]) {
                if (parse_app_name_info(info->attrs[IDRN_ATTR_IPC_PROCESS_NAME],
                                        msg_attrs->ipcp_name) < 0)
                        goto parse_fail;
        }

        if (info->attrs[IDRN_ATTR_DIF_NAME]) {
                if (parse_app_name_info(info->attrs[IDRN_ATTR_DIF_NAME],
                                        msg_attrs->dif_name) < 0)
                        goto parse_fail;
        }

        if (info->attrs[IDRN_ATTR_REGISTRATION])
                msg_attrs->is_registered = \
                        nla_get_flag(info->attrs[IDRN_ATTR_REGISTRATION]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIF"));
        return -1;
}

static int rnl_parse_ipcm_ipcp_dif_unreg_noti_msg(struct genl_info * info,
                                                  struct rnl_ipcm_ipcp_dif_unreg_noti_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               &(msg_attrs->result));
}

static int rnl_parse_ipcm_alloc_flow_req_msg(struct genl_info * info,
                                             struct rnl_ipcm_alloc_flow_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRM_ATTR_MAX + 1];
        struct nlattr *attrs[IAFRM_ATTR_MAX + 1];
        int result;

        attr_policy[IAFRM_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRM_ATTR_SOURCE_APP_NAME].len  = 0;
        attr_policy[IAFRM_ATTR_DEST_APP_NAME].type   = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DEST_APP_NAME].len    = 0;
        attr_policy[IAFRM_ATTR_FLOW_SPEC].type       = NLA_NESTED;
        attr_policy[IAFRM_ATTR_FLOW_SPEC].len        = 0;
        attr_policy[IAFRM_ATTR_DIF_NAME].type        = NLA_NESTED;
        attr_policy[IAFRM_ATTR_DIF_NAME].len         = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IAFRM_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IAFRM_ATTR_SOURCE_APP_NAME],
                                msg_attrs->source) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRM_ATTR_DEST_APP_NAME],
                                msg_attrs->dest) < 0)
                goto parse_fail;

        if (parse_flow_spec(attrs[IAFRM_ATTR_FLOW_SPEC],
                            msg_attrs->fspec) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRM_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ALLOCATE_FLOW_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_alloc_flow_req_arrived_msg(struct genl_info * info,
                                                     struct rnl_ipcm_alloc_flow_req_arrived_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRA_ATTR_MAX + 1];
        struct nlattr *attrs[IAFRA_ATTR_MAX + 1];
        int result;

        attr_policy[IAFRA_ATTR_SOURCE_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_SOURCE_APP_NAME].len = 0;
        attr_policy[IAFRA_ATTR_DEST_APP_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_DEST_APP_NAME].len = 0;
        attr_policy[IAFRA_ATTR_FLOW_SPEC].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_FLOW_SPEC].len = 0;
        attr_policy[IAFRA_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IAFRA_ATTR_DIF_NAME].len = 0;
        attr_policy[IAFRA_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[IAFRA_ATTR_PORT_ID].len = 4;


        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IAFRA_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IAFRA_ATTR_SOURCE_APP_NAME],
                                msg_attrs->source) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRA_ATTR_DEST_APP_NAME],
                                msg_attrs->dest) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IAFRA_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        if (parse_flow_spec(attrs[IAFRA_ATTR_FLOW_SPEC],
                            msg_attrs->fspec) < 0)
                goto parse_fail;

        if (attrs[IAFRA_ATTR_PORT_ID])
                msg_attrs->id =
                        nla_get_u32(attrs[IAFRA_ATTR_PORT_ID]);
        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED"));
        return -1;
}

static int rnl_parse_ipcm_alloc_flow_resp_msg(struct genl_info * info,
                                              struct rnl_alloc_flow_resp_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IAFRE_ATTR_MAX + 1];
        struct nlattr *attrs[IAFRE_ATTR_MAX + 1];
        int result;

        attr_policy[IAFRE_ATTR_RESULT].type = NLA_U32;
        attr_policy[IAFRE_ATTR_RESULT].len = 4;
        attr_policy[IAFRE_ATTR_NOTIFY_SOURCE].type = NLA_FLAG;
        attr_policy[IAFRE_ATTR_NOTIFY_SOURCE].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IAFRE_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[IAFRE_ATTR_RESULT])
                msg_attrs->result =
                        nla_get_u32(attrs[IAFRE_ATTR_RESULT]);

        if (attrs[IAFRE_ATTR_NOTIFY_SOURCE])
                msg_attrs->notify_src =
                        nla_get_flag(attrs[IAFRE_ATTR_NOTIFY_SOURCE]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE"));
        return -1;
}

static int rnl_parse_ipcm_dealloc_flow_req_msg(struct genl_info * info,
                                               struct rnl_ipcm_dealloc_flow_req_msg_attrs * msg_attrs)
{
        return rnl_parse_generic_u32_param_msg(info,
                                               (uint_t *) &(msg_attrs->id));

}

static int rnl_parse_ipcm_flow_dealloc_noti_msg(struct genl_info * info,
                                                struct rnl_ipcm_flow_dealloc_noti_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IFDN_ATTR_MAX + 1];
        struct nlattr *attrs[IFDN_ATTR_MAX + 1];
        int result;

        attr_policy[IFDN_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[IFDN_ATTR_PORT_ID].len = 4;
        attr_policy[IFDN_ATTR_CODE].type = NLA_U32;
        attr_policy[IFDN_ATTR_CODE].len = 4;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IFDN_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[IFDN_ATTR_PORT_ID])
                msg_attrs->id = nla_get_u32(attrs[IFDN_ATTR_PORT_ID]);

        if (attrs[IFDN_ATTR_CODE])
                msg_attrs->code = nla_get_u32(attrs[IFDN_ATTR_CODE]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION"));
        return -1;
}

static int rnl_parse_ipcm_conn_create_req_msg(struct genl_info * info,
                                              struct rnl_ipcp_conn_create_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICCRQ_ATTR_MAX + 1];
        struct nlattr *attrs[ICCRQ_ATTR_MAX + 1];
        int    result;

        attr_policy[ICCRQ_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_PORT_ID].len = 4;
        attr_policy[ICCRQ_ATTR_SOURCE_ADDR].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_SOURCE_ADDR].len = 4;
        attr_policy[ICCRQ_ATTR_DEST_ADDR].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_DEST_ADDR].len = 4;
        attr_policy[ICCRQ_ATTR_QOS_ID].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_QOS_ID].len = 4;
        attr_policy[ICCRQ_ATTR_POLICIES].type = NLA_U32;
        attr_policy[ICCRQ_ATTR_POLICIES].len = 4;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICCRQ_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[ICCRQ_ATTR_PORT_ID])
                msg_attrs->port_id = nla_get_u32(attrs[ICCRQ_ATTR_PORT_ID]);

        if (attrs[ICCRQ_ATTR_SOURCE_ADDR])
                msg_attrs->src_addr = nla_get_u32(attrs[ICCRQ_ATTR_SOURCE_ADDR]);

        if (attrs[ICCRQ_ATTR_DEST_ADDR])
                msg_attrs->dst_addr = nla_get_u32(attrs[ICCRQ_ATTR_DEST_ADDR]);

        if (attrs[ICCRQ_ATTR_QOS_ID])
                msg_attrs->qos_id = nla_get_u32(attrs[ICCRQ_ATTR_QOS_ID]);

        if (attrs[ICCRQ_ATTR_POLICIES])
                msg_attrs->policies = nla_get_u32(attrs[ICCRQ_ATTR_POLICIES]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_CREATE_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_conn_create_arrived_msg(struct genl_info * info,
                                                  struct rnl_ipcp_conn_create_arrived_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICCA_ATTR_MAX + 1];
        struct nlattr *attrs[ICCA_ATTR_MAX + 1];
        int    result;

        attr_policy[ICCA_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[ICCA_ATTR_PORT_ID].len = 4;
        attr_policy[ICCA_ATTR_SOURCE_ADDR].type = NLA_U32;
        attr_policy[ICCA_ATTR_SOURCE_ADDR].len = 4;
        attr_policy[ICCA_ATTR_DEST_ADDR].type = NLA_U32;
        attr_policy[ICCA_ATTR_DEST_ADDR].len = 4;
        attr_policy[ICCA_ATTR_DEST_CEP_ID].type = NLA_U32;
        attr_policy[ICCA_ATTR_DEST_CEP_ID].len = 4;
        attr_policy[ICCA_ATTR_QOS_ID].type = NLA_U32;
        attr_policy[ICCA_ATTR_QOS_ID].len = 4;
        attr_policy[ICCA_ATTR_FLOW_USER_IPC_PROCESS_ID].type = NLA_U16;
        attr_policy[ICCA_ATTR_FLOW_USER_IPC_PROCESS_ID].len = 2;
        attr_policy[ICCA_ATTR_POLICIES].type = NLA_U32;
        attr_policy[ICCA_ATTR_POLICIES].len = 4;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICCA_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[ICCA_ATTR_PORT_ID])
                msg_attrs->port_id =
                        nla_get_u32(attrs[ICCA_ATTR_PORT_ID]);

        if (attrs[ICCA_ATTR_SOURCE_ADDR])
                msg_attrs->src_addr =
                        nla_get_u32(attrs[ICCA_ATTR_SOURCE_ADDR]);

        if (attrs[ICCA_ATTR_DEST_ADDR])
                msg_attrs->dst_addr =
                        nla_get_u32(attrs[ICCA_ATTR_DEST_ADDR]);

        if (attrs[ICCA_ATTR_DEST_CEP_ID])
                msg_attrs->dst_cep =
                        nla_get_u32(attrs[ICCA_ATTR_DEST_CEP_ID]);

        if (attrs[ICCA_ATTR_QOS_ID])
                msg_attrs->qos_id =
                        nla_get_u32(attrs[ICCA_ATTR_QOS_ID]);

        if (attrs[ICCA_ATTR_FLOW_USER_IPC_PROCESS_ID])
                msg_attrs->flow_user_ipc_process_id =
                        nla_get_u16(attrs[ICCA_ATTR_FLOW_USER_IPC_PROCESS_ID]);

        if (attrs[ICCA_ATTR_POLICIES])
                msg_attrs->policies =
                        nla_get_u32(attrs[ICCA_ATTR_POLICIES]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_CREATE_ARRIVED"));
        return -1;
}

static int rnl_parse_ipcm_conn_update_req_msg(struct genl_info * info,
                                              struct rnl_ipcp_conn_update_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICURQ_ATTR_MAX + 1];
        struct nlattr *attrs[ICURQ_ATTR_MAX + 1];
        int    result;

        attr_policy[ICURQ_ATTR_PORT_ID].type                  = NLA_U32;
        attr_policy[ICURQ_ATTR_PORT_ID].len                   = 4;
        attr_policy[ICURQ_ATTR_SOURCE_CEP_ID].type            = NLA_U32;
        attr_policy[ICURQ_ATTR_SOURCE_CEP_ID].len             = 4;
        attr_policy[ICURQ_ATTR_DEST_CEP_ID].type              = NLA_U32;
        attr_policy[ICURQ_ATTR_DEST_CEP_ID].len               = 4;
        attr_policy[ICURQ_ATTR_FLOW_USER_IPC_PROCESS_ID].type = NLA_U16;
        attr_policy[ICURQ_ATTR_FLOW_USER_IPC_PROCESS_ID].len  = 2;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICURQ_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[ICURQ_ATTR_PORT_ID])
                msg_attrs->port_id =
                        nla_get_u32(attrs[ICURQ_ATTR_PORT_ID]);

        if (attrs[ICURQ_ATTR_SOURCE_CEP_ID])
                msg_attrs->src_cep =
                        nla_get_u32(attrs[ICURQ_ATTR_SOURCE_CEP_ID]);

        if (attrs[ICURQ_ATTR_DEST_CEP_ID])
                msg_attrs->dst_cep =
                        nla_get_u32(attrs[ICURQ_ATTR_DEST_CEP_ID]);

        if (attrs[ICURQ_ATTR_FLOW_USER_IPC_PROCESS_ID])
                msg_attrs->flow_user_ipc_process_id =
                        nla_get_u16(attrs[ICURQ_ATTR_FLOW_USER_IPC_PROCESS_ID]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_UPDATE_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_conn_destroy_req_msg(struct genl_info * info,
                                               struct rnl_ipcp_conn_destroy_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[ICDR_ATTR_MAX + 1];
        struct nlattr *attrs[ICDR_ATTR_MAX + 1];
        int    result;

        attr_policy[ICDR_ATTR_PORT_ID].type = NLA_U32;
        attr_policy[ICDR_ATTR_PORT_ID].len = 0;
        attr_policy[ICDR_ATTR_SOURCE_CEP_ID].type = NLA_U32;
        attr_policy[ICDR_ATTR_SOURCE_CEP_ID].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             ICDR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[ICDR_ATTR_PORT_ID])
                msg_attrs->port_id =
                        nla_get_u32(attrs[ICDR_ATTR_PORT_ID]);

        if (attrs[ICDR_ATTR_SOURCE_CEP_ID])
                msg_attrs->src_cep =
                        nla_get_u32(attrs[ICDR_ATTR_SOURCE_CEP_ID]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCP_CONN_DESTROY_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_reg_app_req_msg(struct genl_info * info,
                                          struct rnl_ipcm_reg_app_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IRAR_ATTR_MAX + 1];
        struct nlattr *attrs[IRAR_ATTR_MAX + 1];
        int result;

        attr_policy[IRAR_ATTR_APP_NAME].type = NLA_NESTED;
        attr_policy[IRAR_ATTR_APP_NAME].len = 0;
        attr_policy[IRAR_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IRAR_ATTR_DIF_NAME].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IRAR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IRAR_ATTR_APP_NAME],
                                msg_attrs->app_name) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IRAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_REGISTER_APPLICATION_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_unreg_app_req_msg(struct genl_info * info,
                                            struct rnl_ipcm_unreg_app_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IUAR_ATTR_MAX + 1];
        struct nlattr *attrs[IUAR_ATTR_MAX + 1];
        int result;

        attr_policy[IUAR_ATTR_APP_NAME].type = NLA_NESTED;
        attr_policy[IUAR_ATTR_APP_NAME].len = 0;
        attr_policy[IUAR_ATTR_DIF_NAME].type = NLA_NESTED;
        attr_policy[IUAR_ATTR_DIF_NAME].len = 0;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IUAR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (parse_app_name_info(attrs[IUAR_ATTR_APP_NAME],
                                msg_attrs->app_name) < 0)
                goto parse_fail;

        if (parse_app_name_info(attrs[IUAR_ATTR_DIF_NAME],
                                msg_attrs->dif_name) < 0)
                goto parse_fail;

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST"));
        return -1;
}

static int rnl_parse_ipcm_query_rib_req_msg(struct genl_info * info,
                                            struct rnl_ipcm_query_rib_req_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[IDQR_ATTR_MAX + 1];
        struct nlattr *attrs[IDQR_ATTR_MAX + 1];
        int result;

        attr_policy[IDQR_ATTR_OBJECT].type = NLA_NESTED;
        attr_policy[IDQR_ATTR_SCOPE].type  = NLA_U32;
        attr_policy[IDQR_ATTR_FILTER].type = NLA_STRING;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             IDQR_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Could not validate nl message policy (error = %d)",
                        result);
                goto parse_fail;
        }

        if (attrs[IDQR_ATTR_OBJECT]) {
                if (parse_rib_object(attrs[IDQR_ATTR_OBJECT],
                                     msg_attrs->rib_obj) < 0)
                        goto parse_fail;
        }

        if (attrs[IDQR_ATTR_SCOPE])
                msg_attrs->scope = \
                        nla_get_u32(attrs[IDQR_ATTR_SCOPE]);

        if (attrs[IDQR_ATTR_FILTER])
                nla_strlcpy(msg_attrs->filter,
                            attrs[IDQR_ATTR_FILTER],
                            sizeof(attrs[IDQR_ATTR_FILTER]));
        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_IPCM_QUERY_RIB_REQUEST"));
        return -1;
}

static int parse_list_of_pfte_config_entries(struct nlattr *     nested_attr,
                                             struct rnl_rmt_mod_pfte_msg_attrs * msg)
{
        struct nlattr *       nla;
        struct pdu_ft_entry * entry;
        int                   rem                   = 0;
        int                   entries_with_problems = 0;
        int                   total_entries         = 0;

        if (!nested_attr) {
                LOG_ERR("Bogus attribute passed, bailing out");
                return -1;
        }

        if (!msg) {
                LOG_ERR("Bogus msg passed, bailing out");
                return -1;
        }

        for (nla = (struct nlattr*) nla_data(nested_attr),
                     rem = nla_len(nested_attr);
             nla_ok(nla, rem);
             nla = nla_next(nla, &(rem))) {
                total_entries++;

                entry = rkzalloc(sizeof(*entry), GFP_KERNEL);
                if (!entry) {
                        entries_with_problems++;
                        continue;
                }
                INIT_LIST_HEAD(&entry->next);

                if (parse_pdu_fte_list_entry(nla, entry) < 0) {
                        rkfree(entry);
                        entries_with_problems++;
                        continue;
                }

                list_add(&entry->next, &msg->pft_entries);
        }

        if (rem > 0) {
                LOG_WARN("Missing bits to parse");
        }

        if (entries_with_problems > 0)
                LOG_WARN("Problems parsing %d out of %d parameters",
                         entries_with_problems,
                         total_entries);

        return 0;
}

static int rnl_parse_rmt_modify_fte_req_msg(struct genl_info * info,
                                            struct rnl_rmt_mod_pfte_msg_attrs * msg_attrs)
{
        struct nla_policy attr_policy[RMPFE_ATTR_MAX + 1];
        struct nlattr *attrs[RMPFE_ATTR_MAX + 1];
        int result;

        attr_policy[RMPFE_ATTR_ENTRIES].type = NLA_NESTED;
        attr_policy[RMPFE_ATTR_ENTRIES].len = 0;
        attr_policy[RMPFE_ATTR_MODE].type = NLA_U32;
        attr_policy[RMPFE_ATTR_MODE].len = 4;

        result = nlmsg_parse(info->nlhdr,
                             sizeof(struct genlmsghdr) +
                             sizeof(struct rina_msg_hdr),
                             attrs,
                             RMPFE_ATTR_MAX,
                             attr_policy);

        if (result < 0) {
                LOG_ERR("Error %d; could not validate nl message policy",
                        result);
                goto parse_fail;
        }

        if (attrs[RMPFE_ATTR_ENTRIES]) {
                if (parse_list_of_pfte_config_entries(
                                                      attrs[RMPFE_ATTR_ENTRIES],
                                                      msg_attrs) < 0)
                        goto parse_fail;
        }

        if (attrs[RMPFE_ATTR_MODE])
                msg_attrs->mode =
                        nla_get_u32(attrs[RMPFE_ATTR_MODE]);

        return 0;

 parse_fail:
        LOG_ERR(BUILD_STRERROR_BY_MTYPE("RINA_C_RMT_MODIFY_FTE_REQ_MSG"));
        return -1;
}

int rnl_parse_msg(struct genl_info * info,
                  struct rnl_msg   * msg)
{

        LOG_DBG("RINA Netlink parser started ...");

        if (!info) {
                LOG_ERR("Got empty info, bailing out");
                return -1;
        }

        if (!msg) {
                LOG_ERR("Got empty message, bailing out");
                return -1;
        }

        msg->src_port              = info->snd_portid;
        /* dst_port can not be parsed */
        msg->dst_port              = 0;
        msg->seq_num               = info->snd_seq;
        msg->op_code               = (msg_type_t) info->genlhdr->cmd;
#if 0
        msg->req_msg_flag          = 0;
        msg->resp_msg_flag         = 0;
        msg->notification_msg_flag = 0;
#endif
        msg->header = *((struct rina_msg_hdr *) info->userhdr);

        LOG_DBG("msg at %pK / msg->attrs at %pK",  msg, msg->attrs);
        LOG_DBG("  src-ipc-id: %d", msg->header.src_ipc_id);
        LOG_DBG("  dst-ipc-id: %d", msg->header.dst_ipc_id);

        switch(info->genlhdr->cmd) {
        case RINA_C_IPCM_ASSIGN_TO_DIF_REQUEST:
                if (rnl_parse_ipcm_assign_to_dif_req_msg(info,
                                                         msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_UPDATE_DIF_CONFIG_REQUEST:
                if (rnl_parse_ipcm_update_dif_config_req_msg(info,
                                                             msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_IPC_PROCESS_DIF_REGISTRATION_NOTIFICATION:
                if (rnl_parse_ipcm_ipcp_dif_reg_noti_msg(info,
                                                         msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_IPC_PROCESS_DIF_UNREGISTRATION_NOTIFICATION:
                if (rnl_parse_ipcm_ipcp_dif_unreg_noti_msg(info,
                                                           msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST:
                if (rnl_parse_ipcm_alloc_flow_req_msg(info,
                                                      msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED:
                if (rnl_parse_ipcm_alloc_flow_req_arrived_msg(info,
                                                              msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_ALLOCATE_FLOW_RESPONSE:
                if (rnl_parse_ipcm_alloc_flow_resp_msg(info,
                                                       msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_DEALLOCATE_FLOW_REQUEST:
                if (rnl_parse_ipcm_dealloc_flow_req_msg(info,
                                                        msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION:
                if (rnl_parse_ipcm_flow_dealloc_noti_msg(info,
                                                         msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_CONN_CREATE_REQUEST:
                if (rnl_parse_ipcm_conn_create_req_msg(info,
                                                       msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_CONN_CREATE_ARRIVED:
                if (rnl_parse_ipcm_conn_create_arrived_msg(info,
                                                           msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_CONN_UPDATE_REQUEST:
                if (rnl_parse_ipcm_conn_update_req_msg(info,
                                                       msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCP_CONN_DESTROY_REQUEST:
                if (rnl_parse_ipcm_conn_destroy_req_msg(info,
                                                        msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_REGISTER_APPLICATION_REQUEST:
                if (rnl_parse_ipcm_reg_app_req_msg(info,
                                                   msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_UNREGISTER_APPLICATION_REQUEST:
                if (rnl_parse_ipcm_unreg_app_req_msg(info,
                                                     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_IPCM_QUERY_RIB_REQUEST:
                if (rnl_parse_ipcm_query_rib_req_msg(info,
                                                     msg->attrs) < 0)
                        goto fail;
                break;
        case RINA_C_RMT_MODIFY_FTE_REQUEST:
                if (rnl_parse_rmt_modify_fte_req_msg(info,
                                                     msg->attrs) < 0)
                        goto fail;
                break;
        default:
                goto fail;
                break;
        }
        return 0;

 fail:
        LOG_ERR("Could not parse NL message type: %d", info->genlhdr->cmd);
        return -1;
}
EXPORT_SYMBOL(rnl_parse_msg);

/* FORMATTING */
static int format_fail(char * msg_name)
{
        LOG_ERR("Could not format %s message correctly", msg_name);
        return -1;
}

static int format_app_name_info(const struct name * name,
                                struct sk_buff *    msg)
{
        if (!msg || !name) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        /*
         * Components might be missing (and nla_put_string wonna have NUL
         * terminated strings, otherwise kernel panics are on the way).
         * Would we like to fallback here or simply return an error if (at
         * least) one of them is missing ?
         */

        if (name->process_name)
                if (nla_put_string(msg,
                                   APNI_ATTR_PROCESS_NAME,
                                   name->process_name))
                        return -1;
        if (name->process_instance)
                if (nla_put_string(msg,
                                   APNI_ATTR_PROCESS_INSTANCE,
                                   name->process_instance))
                        return -1;
        if (name->entity_name)
                if (nla_put_string(msg,
                                   APNI_ATTR_ENTITY_NAME,
                                   name->entity_name))
                        return -1;
        if (name->entity_instance)
                if (nla_put_string(msg,
                                   APNI_ATTR_ENTITY_INSTANCE,
                                   name->entity_instance))
                        return -1;

        return 0;
}

static int format_flow_spec(const struct flow_spec * fspec,
                            struct sk_buff *         msg)
{
        if (!fspec) {
                LOG_ERR("Cannot format flow-spec, "
                        "fspec parameter is NULL ...");
                return -1;
        }
        if (!msg) {
                LOG_ERR("Cannot format flow-spec, "
                        "message parameter is NULL ...");
                return -1;
        }

        /*
         * FIXME: only->max or min attributes are taken from
         *  uint_range types
         */

        /* FIXME: ??? only max is accessed, what do you mean ? */

        /* FIXME: librina does not define ranges for these attributes, just
         * unique values. So far I seleced only the max or min value depending
         * on the most restrincting (in this case all max).
         * Leo */

        if (fspec->average_bandwidth > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_BWITH,
                                fspec->average_bandwidth))
                        return -1;
        if (fspec->average_sdu_bandwidth > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_AVG_SDU_BWITH,
                                fspec->average_sdu_bandwidth))
                        return -1;
        if (fspec->delay > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_DELAY,
                                fspec->delay))
                        return -1;
        if (fspec->jitter > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_JITTER,
                                fspec->jitter))
                        return -1;
        if (fspec->max_allowable_gap >= 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_GAP,
                                fspec->max_allowable_gap))
                        return -1;
        if (fspec->max_sdu_size > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_MAX_SDU_SIZE,
                                fspec->max_sdu_size))
                        return -1;
        if (fspec->ordered_delivery)
                if (nla_put_flag(msg,
                                 FSPEC_ATTR_IN_ORD_DELIVERY))
                        return -1;
        if (fspec->partial_delivery)
                if (nla_put_flag(msg,
                                 FSPEC_ATTR_PART_DELIVERY))
                        return -1;
        if (fspec->peak_bandwidth_duration > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_BWITH_DURATION,
                                fspec->peak_bandwidth_duration))
                        return -1;
        if (fspec->peak_sdu_bandwidth_duration > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_PEAK_SDU_BWITH_DURATION,
                                fspec->peak_sdu_bandwidth_duration))
                        return -1;
        if (fspec->undetected_bit_error_rate > 0)
                if (nla_put_u32(msg,
                                FSPEC_ATTR_UNDETECTED_BER,
                                fspec->undetected_bit_error_rate))
                        return -1;

        return 0;
}

static int rnl_format_generic_u32_param_msg(u32              param_var,
                                            uint_t           param_name,
                                            string_t *       msg_name,
                                            struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, param_name, param_var) < 0) {
                LOG_ERR("Could not format %s message correctly", msg_name);
                return -1;
        }
        return 0;
}

static int rnl_format_ipcm_assign_to_dif_resp_msg(uint_t          result,
                                                  struct sk_buff  * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IAFRRM_ATTR_RESULT,
                                                "rnl_ipcm_assign_"
                                                "to_dif_resp_msg",
                                                skb_out);
}

static int rnl_format_ipcm_update_dif_config_resp_msg(uint_t          result,
                                                      struct sk_buff  * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IAFRRM_ATTR_RESULT,
                                                "rnl_ipcm_update_dif_"
                                                "config_resp_msg",
                                                skb_out);
}

static int 
rnl_format_ipcm_alloc_flow_req_arrived_msg(const struct name *      source,
                                           const struct name *      dest,
                                           const struct flow_spec * fspec,
                                           const struct name *      dif_name,
                                           port_id_t                pid,
                                           struct sk_buff *         skb_out)
{
        struct nlattr * msg_src_name, * msg_dst_name;
        struct nlattr * msg_fspec,    * msg_dif_name;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        /* name-formating might be moved into its own function (and reused) */
        if (!(msg_src_name =
              nla_nest_start(skb_out, IAFRA_ATTR_SOURCE_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_src_name);
                LOG_ERR(BUILD_STRERROR("source application name attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_app_name_info(source, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_src_name);

        if (!(msg_dst_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DEST_APP_NAME))) {
                nla_nest_cancel(skb_out, msg_dst_name);
                LOG_ERR(BUILD_STRERROR("destination app name attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_app_name_info(dest, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_dst_name);

        if (!(msg_dif_name =
              nla_nest_start(skb_out, IAFRA_ATTR_DIF_NAME))) {
                nla_nest_cancel(skb_out, msg_dif_name);
                LOG_ERR(BUILD_STRERROR("DIF name attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_app_name_info(dif_name, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_dif_name);

        if (!(msg_fspec =
              nla_nest_start(skb_out, IAFRA_ATTR_FLOW_SPEC))) {
                nla_nest_cancel(skb_out, msg_fspec);
                LOG_ERR(BUILD_STRERROR("flow spec attribute"));
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");
        }

        if (format_flow_spec(fspec, skb_out) < 0)
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        nla_nest_end(skb_out, msg_fspec);

        if (nla_put_u32(skb_out, IAFRA_ATTR_PORT_ID, pid))
                return format_fail("rnl_ipcm_alloc_flow_req_arrived_msg");

        return 0;
}

static int rnl_format_ipcm_alloc_flow_req_result_msg(uint_t           result,
                                                     port_id_t        pid,
                                                     struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IAFRRM_ATTR_PORT_ID, pid))
                return format_fail("rnl_format_ipcm_alloc_req_result_msg");

        if (nla_put_u32(skb_out, IAFRRM_ATTR_RESULT, result))
                return format_fail("rnl_format_ipcm_alloc_req_result_msg");

        return 0;
}

static int rnl_format_ipcm_dealloc_flow_resp_msg(uint_t           result,
                                                 struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(result,
                                                IDFRE_ATTR_RESULT,
                                                "rnl_ipcm_dealloc_"
                                                "flow_resp_msg",
                                                skb_out);
}

static int rnl_format_ipcm_flow_dealloc_noti_msg(port_id_t        id,
                                                 uint_t           code,
                                                 struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IFDN_ATTR_PORT_ID, id))
                return format_fail("rnl_ipcm_alloc_flow_resp_msg");

        if (nla_put_u32(skb_out, IFDN_ATTR_CODE, code ))
                return format_fail("rnl_ipcm_alloc_flow_resp_msg");

        return 0;
}

static int rnl_format_ipcm_conn_create_resp_msg(port_id_t        id,
                                                cep_id_t         src_cep,
                                                struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICCRE_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_create_resp_msg");

        if (nla_put_u32(skb_out, ICCRE_ATTR_SOURCE_CEP_ID, src_cep ))
                return format_fail("rnl_format_ipcm_conn_create_resp_msg");

        return 0;
}

static int rnl_format_ipcm_conn_create_result_msg(port_id_t        id,
                                                  cep_id_t         src_cep,
                                                  cep_id_t         dst_cep,
                                                  struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICCRS_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_create_result_msg");

        if (nla_put_u32(skb_out, ICCRS_ATTR_SOURCE_CEP_ID, src_cep ))
                return format_fail("rnl_format_ipcm_conn_create_result_msg");

        if (nla_put_u32(skb_out, ICCRS_ATTR_DEST_CEP_ID, dst_cep ))
                return format_fail("rnl_format_ipcm_conn_create_result_msg");

        return 0;
}

static int rnl_format_ipcm_conn_update_result_msg(port_id_t        id,
                                                  uint_t           result,
                                                  struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICURS_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_update_result_msg");

        if (nla_put_u32(skb_out, ICURS_ATTR_RESULT, result))
                return format_fail("rnl_format_ipcm_conn_update_result_msg");

        return 0;
}

static int rnl_format_ipcm_conn_destroy_result_msg(port_id_t        id,
                                                   uint_t           result,
                                                   struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, ICDRS_ATTR_PORT_ID, id))
                return format_fail("rnl_format_ipcm_conn_destroy_result_msg");

        if (nla_put_u32(skb_out, ICDRS_ATTR_RESULT, result))
                return format_fail("rnl_format_ipcm_conn_destroy_result_msg");

        return 0;
}

static int rnl_format_ipcm_reg_app_resp_msg(uint_t           result,
                                            struct sk_buff * skb_out)
{
        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (nla_put_u32(skb_out, IRARE_ATTR_RESULT, result ))
                return format_fail("rnl_ipcm_reg_app_resp_msg");

        return 0;
}

static int rnl_format_socket_closed_notification_msg(u32              nl_port,
                                                     struct sk_buff * skb_out)
{
        return rnl_format_generic_u32_param_msg(nl_port,
                                                ISCN_ATTR_PORT,
                                                "rnl_format_socket_closed_"
                                                "notification_msg",
                                                skb_out);
}

static int send_nl_unicast_msg(struct net *     net,
                               struct sk_buff * skb,
                               u32              portid,
                               msg_type_t       type,
                               rnl_sn_t         seq_num)
{
        int result;

        if (!net) {
                LOG_ERR("Wrong net parameter, cannot send unicast NL message");
                return -1;
        }

        if (!skb) {
                LOG_ERR("Wrong skb parameter, cannot send unicast NL message");
                return -1;
        }

        LOG_DBG("Going to send NL unicast message "
                "(type = %d, seq-num %u, port = %u)",
                (int) type, seq_num, portid);

        result = genlmsg_unicast(net, skb, portid);
        if (result) {
                LOG_ERR("Could not send NL message "
                        "(type = %d, seq-num %u, port = %u, result = %d)",
                        (int) type, seq_num, portid, result);
                nlmsg_free(skb);
                return -1;
        }

        LOG_DBG("Unicast NL message sent "
                "(type = %d, seq-num %u, port = %u)",
                (int) type, seq_num, portid);

        return 0;
}

static int format_pft_entry_port_list(port_id_t *      ports,
                                      size_t           ports_size,
                                      struct sk_buff * skb_out)
{
        struct nlattr * msg_ports;
        int i = 0;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        if (!(msg_ports =
              nla_nest_start(skb_out, PFTELE_ATTR_PORTIDS))) {
                nla_nest_cancel(skb_out, msg_ports);
                return -1;
        }

        for (i = 0; i < ports_size; i++) {
                if (nla_put_u32(skb_out, i, ports[i]))
                        return -1;
        }

        nla_nest_end(skb_out, msg_ports);
        return 0;
}

static int format_pft_entries_list(struct list_head * entries,
                                   struct sk_buff *   skb_out)
{
        struct nlattr * msg_entry;
        struct pdu_ft_entry * pos, *nxt;
        int i = 0;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }

        list_for_each_entry_safe(pos, nxt, entries, next) {
                i++;
                if (!(msg_entry =
                      nla_nest_start(skb_out, i))) {
                        nla_nest_cancel(skb_out, msg_entry);
                        LOG_ERR(BUILD_STRERROR("pft entries list attribute"));
                        return format_fail("rnl_ipcm_pft_dump_resp_msg");
                }

                if ((nla_put_u32(skb_out, 
                                 PFTELE_ATTR_ADDRESS,
                                 pos->destination)                         ||
                     nla_put_u32(skb_out, PFTELE_ATTR_QOSID, pos->qos_id)) ||
                     format_pft_entry_port_list(pos->ports,
                                                pos->ports_size,
                                                skb_out))
                        return format_fail("rnl_ipcm_pft_dump_resp_msg");
                
                nla_nest_end(skb_out, msg_entry);
                list_del(&pos->next);
                rkfree(pos->ports);
                rkfree(pos);
        }
                      
        return 0;
}

static int rnl_format_ipcm_pft_dump_resp_msg(int                result,
                                             struct list_head * entries,
                                             struct sk_buff *   skb_out)
{
        struct nlattr * msg_entries;

        if (!skb_out) {
                LOG_ERR("Bogus input parameter(s), bailing out");
                return -1;
        }
        
        if (nla_put_u32(skb_out, RPFD_ATTR_RESULT, result) < 0)
                return format_fail("rnl_ipcm_pft_dump_resp_msg");

        if (!(msg_entries =
              nla_nest_start(skb_out, RPFD_ATTR_ENTRIES))) {
                nla_nest_cancel(skb_out, msg_entries);
                LOG_ERR(BUILD_STRERROR("pft entries list attribute"));
                return format_fail("rnl_ipcm_pft_dump_resp_msg");
        }
        if (format_pft_entries_list(entries, skb_out))
                return format_fail("rnl_ipcm_pft_dump_resp_msg");
        nla_nest_end(skb_out, msg_entries);

        return 0;
}


int rnl_assign_dif_response(ipc_process_id_t id,
                            uint_t           res,
                            rnl_sn_t         seq_num,
                            u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_assign_to_dif_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_ASSIGN_TO_DIF_RESPONSE,
                                   seq_num);

}
EXPORT_SYMBOL(rnl_assign_dif_response);

int rnl_update_dif_config_response(ipc_process_id_t id,
                                   uint_t           res,
                                   rnl_sn_t         seq_num,
                                   u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_update_dif_config_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_UPDATE_DIF_CONFIG_RESPONSE,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_update_dif_config_response);

int rnl_app_register_unregister_response_msg(ipc_process_id_t ipc_id,
                                             uint_t           res,
                                             rnl_sn_t         seq_num,
                                             u32              nl_port_id,
                                             bool             isRegister)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        uint_t                command;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        command = isRegister                               ?
                RINA_C_IPCM_REGISTER_APPLICATION_RESPONSE  :
                RINA_C_IPCM_UNREGISTER_APPLICATION_RESPONSE;

        out_hdr = (struct rina_msg_hdr *) genlmsg_put(out_msg,
                                                      0,
                                                      seq_num,
                                                      &rnl_nl_family,
                                                      0,
                                                      command);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_reg_app_resp_msg(res, out_msg)) {
                LOG_ERR("Could not format message ...");
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   command,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_register_unregister_response_msg);

int rnl_app_alloc_flow_req_arrived_msg(ipc_process_id_t         ipc_id,
                                       const struct name *      dif_name,
                                       const struct name *      source,
                                       const struct name *      dest,
                                       const struct flow_spec * fspec,
                                       rnl_sn_t                 seq_num,
                                       u32                      nl_port_id,
                                       port_id_t                pid)
{
        struct sk_buff * msg;
        struct rina_msg_hdr * hdr;
        int result;

        msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        hdr = (struct rina_msg_hdr *)
                genlmsg_put(msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            NLM_F_REQUEST,
                            RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED);
        if (!hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(msg);
                return -1;
        }

        hdr->dst_ipc_id = 0;
        hdr->src_ipc_id = ipc_id;
        if (rnl_format_ipcm_alloc_flow_req_arrived_msg(source,
                                                       dest,
                                                       fspec,
                                                       dif_name,
                                                       pid,
                                                       msg)) {
                nlmsg_free(msg);
                return -1;
        }

        result = genlmsg_end(msg, hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   msg,
                                   nl_port_id,
                                   RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_ARRIVED,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_alloc_flow_req_arrived_msg);

int rnl_app_alloc_flow_result_msg(ipc_process_id_t ipc_id,
                                  uint_t           res,
                                  port_id_t        pid,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_alloc_flow_req_result_msg(res, pid, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_ALLOCATE_FLOW_REQUEST_RESULT,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_alloc_flow_result_msg);

int rnl_app_dealloc_flow_resp_msg(ipc_process_id_t ipc_id,
                                  uint_t           res,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_dealloc_flow_resp_msg(res, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_DEALLOCATE_FLOW_RESPONSE,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_app_dealloc_flow_resp_msg);

int rnl_flow_dealloc_not_msg(ipc_process_id_t ipc_id,
                             uint_t           code,
                             port_id_t        port_id,
                             u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            0,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_flow_dealloc_noti_msg(port_id, code, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCM_FLOW_DEALLOCATED_NOTIFICATION,
                                   0);
}
EXPORT_SYMBOL(rnl_flow_dealloc_not_msg);

int rnl_ipcp_conn_create_resp_msg(ipc_process_id_t ipc_id,
                                  port_id_t        pid,
                                  cep_id_t         src_cep,
                                  rnl_sn_t         seq_num,
                                  u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_CREATE_RESPONSE);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_create_resp_msg(pid, src_cep, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

#if 0
        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);
#endif

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_CONN_CREATE_RESPONSE,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_ipcp_conn_create_resp_msg);

int rnl_ipcp_conn_create_result_msg(ipc_process_id_t ipc_id,
                                    port_id_t        pid,
                                    cep_id_t         src_cep,
                                    cep_id_t         dst_cep,
                                    rnl_sn_t         seq_num,
                                    u32              nl_port_id)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_CREATE_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_create_result_msg(pid,
                                                   src_cep, dst_cep,
                                                   out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_CONN_CREATE_RESULT,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_ipcp_conn_create_result_msg);

int rnl_ipcp_conn_update_result_msg(ipc_process_id_t ipc_id,
                                    port_id_t        pid,
                                    uint_t           res,
                                    rnl_sn_t         seq_num,
                                    u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int    result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_UPDATE_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_update_result_msg(pid, res, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   nl_port_id,
                                   RINA_C_IPCP_CONN_UPDATE_RESULT,
                                   seq_num);
}
EXPORT_SYMBOL(rnl_ipcp_conn_update_result_msg);

int rnl_ipcp_conn_destroy_result_msg(ipc_process_id_t ipc_id,
                                     port_id_t        pid,
                                     uint_t           res,
                                     rnl_sn_t         seq_num,
                                     u32              nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;
        int    result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            seq_num,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCP_CONN_DESTROY_RESULT);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_ipcm_conn_destroy_result_msg(pid, res, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }
        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);
        if (result) {
                LOG_ERR("Could not send unicast msg: %d", result);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(rnl_ipcp_conn_destroy_result_msg);

int rnl_ipcm_sock_closed_notif_msg(u32 closed_port, u32 dest_port)
{
        struct sk_buff *      out_msg;
        struct rina_msg_hdr * out_hdr;
        int                   result;

        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,
                            0,
                            &rnl_nl_family,
                            0,
                            RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }

        out_hdr->src_ipc_id = 0;
        out_hdr->dst_ipc_id = 0;

        if (rnl_format_socket_closed_notification_msg(closed_port, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }

        result = genlmsg_end(out_msg, out_hdr);
        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }

        return send_nl_unicast_msg(&init_net,
                                   out_msg,
                                   dest_port,
                                   RINA_C_IPCM_SOCKET_CLOSED_NOTIFICATION,
                                   0);
}
EXPORT_SYMBOL(rnl_ipcm_sock_closed_notif_msg);

int rnl_ipcp_pft_dump_resp_msg(ipc_process_id_t   ipc_id,
                               int                result, 
                               struct list_head * entries,
                               rnl_sn_t           seq_num,
                               u32                nl_port_id)
{
        struct sk_buff * out_msg;
        struct rina_msg_hdr * out_hdr;

        /*FIXME: Maybe size should be obtained somehow */
        out_msg = genlmsg_new(NLMSG_DEFAULT_SIZE,GFP_ATOMIC);
        if (!out_msg) {
                LOG_ERR("Could not allocate memory for message");
                return -1;
        }    

        out_hdr = (struct rina_msg_hdr *)
                genlmsg_put(out_msg,
                            0,   
                            seq_num,
                            &rnl_nl_family,
                            0,   
                            RINA_C_RMT_DUMP_FT_REPLY);
        if (!out_hdr) {
                LOG_ERR("Could not use genlmsg_put");
                nlmsg_free(out_msg);
                return -1;
        }    

        out_hdr->src_ipc_id = ipc_id; /* This IPC process */
        out_hdr->dst_ipc_id = 0; 

        if (rnl_format_ipcm_pft_dump_resp_msg(result, entries, out_msg)) {
                nlmsg_free(out_msg);
                return -1;
        }    

        result = genlmsg_end(out_msg, out_hdr);

        if (result) {
                LOG_DBG("Result of genlmesg_end: %d", result);
        }    
        result = genlmsg_unicast(&init_net, out_msg, nl_port_id);
        if (result) {
                LOG_ERR("Could not send unicast msg: %d", result);
                return -1;
        }    
        return 0;
}
EXPORT_SYMBOL(rnl_ipcp_pft_dump_resp_msg);
