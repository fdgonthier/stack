/*
 * EFCP (Error and Flow Control Protocol)
 *
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

#include <linux/kobject.h>

#define RINA_PREFIX "efcp"

#include "logs.h"
#include "utils.h"
#include "debug.h"
#include "efcp.h"
#include "efcp-utils.h"
#include "dtp.h"
#include "dtcp.h"

struct efcp {
        struct connection * connection;
        struct dtp *        dtp;
        struct dtcp *       dtcp;
};

static struct efcp * efcp_create(void)
{
        struct efcp * instance;

        instance = rkzalloc(sizeof(*instance), GFP_KERNEL);
        if (!instance) {
                LOG_ERR("Cannot create a new instance");
                return NULL;
        }

        LOG_DBG("Instance %pK initialized successfully", instance);

        return instance;
}

static int efcp_destroy(struct efcp * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (instance->dtp)        dtp_unbind(instance->dtp);
        if (instance->dtcp)       dtcp_unbind(instance->dtcp);

        if (instance->dtp)        dtp_destroy(instance->dtp);
        if (instance->dtcp)       dtcp_destroy(instance->dtcp);

        if (instance->connection) rkfree(instance->connection);

        rkfree(instance);

        LOG_DBG("Instance %pK finalized successfully", instance);

        return 0;
}

struct efcp_container {
        struct efcp_imap *       instances;
        struct connection_imap * connections;
        struct cidm *            cidm;
};

// efcp_imap maps cep_id_t to efcp_instances

struct efcp_container * efcp_container_create(void)
{
        struct efcp_container * container;

        container = rkzalloc(sizeof(*container), GFP_KERNEL);
        if (!container)
                return NULL;

        container->instances   = efcp_imap_create();
        container->connections = connection_imap_create();
        container->cidm        = cidm_create();

        return container;
}
EXPORT_SYMBOL(efcp_container_create);

int efcp_container_destroy(struct efcp_container * container)
{
        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }

        efcp_imap_destroy(container->instances, efcp_destroy);
        rkfree(container);

        return 0;
}
EXPORT_SYMBOL(efcp_container_destroy);

int efcp_container_write(struct efcp_container * container,
                         port_id_t               port_id,
                         struct sdu *            sdu)
{
        struct connection * conn;
        struct efcp *       efcp;

        conn = conn_imap_find(container->connections, port_id);
        if (!conn) {
                LOG_ERR("There is no connection bound to this port_id %d",
                        port_id);
                return -1;
        }

        efcp = efcp_imap_find(container->instances, conn->source_cep_id);
        if (!efcp) {
                LOG_ERR("There is no EFCP bound to this port_id %d", port_id);
                return -1;
        }
        if (efcp_send(efcp, port_id, sdu))
                return -1;

        return 0;
}
EXPORT_SYMBOL(efcp_container_write);

static int is_connection_ok(const struct connection * connection)
{
        if (!connection                                   ||
            !is_cep_id_ok(connection->source_cep_id)      ||
            !is_cep_id_ok(connection->destination_cep_id) ||
            !is_port_id_ok(connection->port_id))
                return 0;

        return 1;
}

cep_id_t efcp_connection_create(struct efcp_container *   container,
                                struct connection * connection)
{
        struct efcp * tmp;
        cep_id_t      cep_id;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }
        if (!is_connection_ok(connection)) {
                LOG_ERR("Bogus connection passed, bailing out");
                return -1;
        }
        ASSERT(connection);

        tmp = efcp_create();
        if (!tmp)
                return -1;

        cep_id = cidm_allocate(container->cidm);
        /* We must ensure that the DTP is instantiated, at least ... */
        connection->source_cep_id = cep_id;
        tmp->connection = connection;
        tmp->dtp        = dtp_create(/* connection->port_id */);
        if (!tmp->dtp) {
                efcp_destroy(tmp);
                return -1;
        }

        /* FIXME: We need to know if DTCP is needed ...
        tmp->dtcp = dtcp_create();
        if (!tmp->dtcp) {
                efcp_destroy(tmp);
                return -1;
        }
        */

        /* No needs to check here, bindings are straightforward */
        dtp_bind(tmp->dtp,   tmp->dtcp);
        /* dtcp_bind(tmp->dtcp, tmp->dtp); */

        if (efcp_imap_add(container->instances,
                          connection->source_cep_id,
                          tmp)) {
                LOG_ERR("Cannot add a new instance into container %pK",
                        container);
                connection_destroy(connection);
                dtp_destroy(tmp->dtp);
                efcp_destroy(tmp);
                return -1;
        }

        if (connection_imap_add(container->connections,
                                connection->port_id,
                                connection)) {
                LOG_ERR("Cannot add a new connection into container %pK",
                        container);
                efcp_imap_remove(container->instances,
                                 connection->source_cep_id);
                connection_destroy(connection);
                dtp_destroy(tmp->dtp);
                efcp_destroy(tmp);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(efcp_connection_create);

int efcp_connection_destroy(struct efcp_container * container,
                            cep_id_t                id)
{
        struct efcp * tmp;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }

        tmp = efcp_imap_find(container->instances, id);
        if (!tmp) {
                LOG_ERR("Cannot find instance %d in container %pK",
                        id, container);
                return -1;
        }

        if (efcp_imap_remove(container->instances, id)) {
                LOG_ERR("Cannot remove instance %d from container %pK",
                        id, container);
                return -1;
        }

        if (efcp_destroy(tmp)) {
                LOG_ERR("Cannot destroy instance %d, instance lost", id);
                return -1;
        }

        return 0;
}
EXPORT_SYMBOL(efcp_connection_destroy);

int efcp_connection_update(struct efcp_container * container,
                           cep_id_t                from,
                           cep_id_t                to)
{
        struct efcp * tmp;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return -1;
        }

        tmp = efcp_imap_find(container->instances, from);
        if (!tmp) {
                LOG_ERR("Cannot get instance %d from container %pK",
                        from, container);
                return -1;
        }
        tmp->connection->destination_cep_id = to;

        return 0;
}
EXPORT_SYMBOL(efcp_connection_update);

struct efcp * efcp_find(struct efcp_container * container,
                        cep_id_t                id)
{
        struct efcp * tmp;

        if (!container) {
                LOG_ERR("Bogus container passed, bailing out");
                return NULL;
        }

        tmp = efcp_imap_find(container->instances, id);

        return tmp;
}

int efcp_send(struct efcp * instance,
              port_id_t     id,
              struct sdu *  sdu)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return -1;
        }

        if (!is_port_id_ok(id)) {
                LOG_ERR("Wrong port-id passed");
                return -1;
        }

        if (!is_sdu_ok(sdu)) {
                LOG_ERR("Bogus SDU passed");
                return -1;
        }

        if (!instance->dtp) {
                LOG_ERR("No DTP instance available, cannot send");
                return -1;
        }

        return dtp_send(instance->dtp, sdu);
}

struct pdu * efcp_receive_pdu(struct efcp * instance)
{
        if (!instance) {
                LOG_ERR("Bogus instance passed, bailing out");
                return NULL;
        }

        return dtp_receive(instance->dtp);
}
