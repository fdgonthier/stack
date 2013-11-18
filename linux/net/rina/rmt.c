/*
 * RMT (Relaying and Multiplexing Task)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Leonardo Bergesio     <leonardo.bergesio@i2cat.net>
 *    Miquel Tarzan         <miquel.tarzan@i2cat.net>
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

#define RINA_PREFIX "rmt"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "rmt.h"
#include "pft.h"
#include "efcp-utils.h"

struct rmt {
        struct pft *              pft; /* The PDU Forwarding Table */
        struct kfa *              kfa;
        struct efcp_container *   efcpc;
        struct workqueue_struct * egress_wq;
        /* HASH_TABLE(queues, port_id_t, rmt_queues_t *); */
};

struct rmt * rmt_create(struct kfa * kfa,
                        struct efcp_container * efcpc)
{
        struct rmt * tmp;

        if (!kfa)
                return NULL;

        tmp = rkzalloc(sizeof(*tmp), GFP_KERNEL);
        if (!tmp)
                return NULL;

        tmp->pft = pft_create();
        if (!tmp->pft) {
                rkfree(tmp);
                return NULL;
        }

        tmp->kfa   = kfa;
        tmp->efcpc = efcpc;

        tmp->egress_wq = rwq_create("rmt-egress-wq");
        if (!tmp->egress_wq) {
                LOG_ERR("Cannot create rmt egress workqueue");
                rmt_destroy(tmp);
                return NULL;
        }
        LOG_DBG("Instance %pK initialized successfully", tmp);

        return tmp;
}
EXPORT_SYMBOL(rmt_create);

int rmt_destroy(struct rmt * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        ASSERT(instance->pft);
        pft_destroy(instance->pft);
        if (instance->egress_wq) rwq_destroy(instance->egress_wq);
        rkfree(instance);

        LOG_DBG("Instance %pK finalized successfully", instance);


        return 0;
}
EXPORT_SYMBOL(rmt_destroy);

struct send_data {
        struct rmt * rmt;
        struct pdu * pdu;
        address_t    address;
        cep_id_t     connection_id;
};

bool is_send_data_complete(const struct send_data * data)
{
        bool ret;

        ret = ((!data || !data->rmt || !data->pdu) ? false : true);

        LOG_DBG("Send data complete? %d", ret);

        return ret;
}

static int send_data_destroy(struct send_data * data)
{
        if (!data) {
                LOG_ERR("No write data passed");
                return -1;
        }

        rkfree(data);

        return 0;
}

static struct send_data * send_data_create(struct rmt * rmt,
                                           struct pdu * pdu,
                                           address_t    address,
                                           cep_id_t     connection_id)
{
        struct send_data * tmp;

        tmp = rkmalloc(sizeof(*tmp), GFP_ATOMIC);
        if (!tmp)
                return NULL;

        tmp->rmt           = rmt;
        tmp->pdu           = pdu;
        tmp->address       = address;
        tmp->connection_id = connection_id;

        return tmp;
}

static int rmt_send_worker(void * o)
{
        struct send_data * tmp;

        tmp = (struct send_data *) o;
        if (!tmp) {
                LOG_ERR("No send data passed");
                return -1;
        }

        if (!is_send_data_complete(tmp)) {
                LOG_ERR("Wrong data passed to RMT send worker");
                send_data_destroy(tmp);
                return -1;
        }

        /*
         * FIXME : Port id will be retrieved from the pduft, and the cast from
         * PDU to SDU might be changed for a better solution
         */
        if (kfa_flow_sdu_write(tmp->rmt->kfa, -1, (struct sdu *) tmp->pdu)) {
                return -1;
        }

        return 0;
}

int rmt_send(struct rmt * instance,
             address_t    address,
             cep_id_t     connection_id,
             struct pdu * pdu)
{
        struct send_data * tmp;
        struct rwq_work_item * item;

        LOG_MISSING;

        if (!instance) {
                LOG_ERR("Bogus RMT passed");
                return -1;
        }
        if (!pdu) {
                LOG_ERR("Bogus PDU passed");
                return -1;
        }
        if (!is_cep_id_ok(connection_id)) {
                LOG_ERR("Bad cep id");
                return -1;
        }

        tmp = send_data_create(instance, pdu, address, connection_id);
        if (!is_send_data_complete(tmp))
                return -1;

        item = rwq_work_create(GFP_ATOMIC, rmt_send_worker, tmp);
        if (!item) {
                send_data_destroy(tmp);
                return -1;
        }

        ASSERT(instance->egress_wq);

        if (rwq_work_post(instance->egress_wq, item)) {
                send_data_destroy(tmp);
                pdu_destroy(pdu);
                return -1;
        }

        return 0;
}

int rmt_sdu_post(struct rmt * instance,
                 struct sdu * sdu,
                 port_id_t    from)
{
        LOG_MISSING;

        /* Examples:
         * kfa_sdu_post_to_user_space(instance->kfa, sdu, from);
         * efcp_container_receive(instance->efcpc, -1, sdu);
         */

        return 0;
}

