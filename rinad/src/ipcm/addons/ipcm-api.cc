/*
 * IPC Manager GRPC API
 * *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#define RINA_PREFIX "ipcm.grpc"
#include <librina/common.h>
#include <librina/logs.h>
#include <librina/timer.h>
#include <librina/console.h>

#include <grpc++/grpc++.h>
#include <grpc++/health_check_service_interface.h>
#include <grpc++/ext/proto_server_reflection_plugin.h>
#include <grpc/support/log.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>

#include "rina-configuration.h"
#include "../ipcm.h"
#include "ipcm-api.h"

#include "proto/ipcm_api.grpc.pb.h"

using namespace rina;

namespace rinad {

grpc::Status IPCMApiService::ListIpcps(grpc::ServerContext* ctx,
                                       const ipcm_api::ListIpcpsReq* req,
                                       ipcm_api::ListIpcpsRes* res) {
    std::list<IPCPDescription> ipcpDescs;
    std::list<IPCPDescription>::const_iterator it;

    IPCManager->list_ipcps(ipcpDescs);

    try {
        for (it = ipcpDescs.begin(); it != ipcpDescs.end(); ++it) {
            ipcm_api::IPCP *ipcp;

            ipcp = res->add_ipcp();
            ipcp->set_id((*it).get_id());
            ipcp->set_name((*it).get_name());
            ipcp->set_type((*it).get_type());

            for (auto appIt = (*it).get_apps().begin(); appIt != (*it).get_apps().end(); ++appIt)
                ipcp->add_apps((*appIt));

            for (auto flowIt = (*it).get_flows().begin(); flowIt != (*it).get_flows().end(); ++flowIt)
                ipcp->add_flows((*flowIt));
        }
    } catch(...) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Exception will listing IPCPs");
    }

    return grpc::Status::OK;
}

grpc::Status IPCMApiService::GetIfaceForDif(grpc::ServerContext* ctx,
                                            const ipcm_api::GetIfaceForDifReq* req,
                                            ipcm_api::GetIfaceForDifRes* res) {
    res->set_iface(IPCManager->get_interface_for_dif(req->dif()));
    return grpc::Status::OK;
}

grpc::Status IPCMApiService::QueryRib(grpc::ServerContext* ctx,
                                      const ipcm_api::QueryRibReq* req,
                                      ipcm_api::QueryRibRes* res) {
    QueryRIBPromise promise;
    std::string object_class, object_name;
    std::ostringstream err;
    std::vector<struct QueryRIBObject>::iterator it;
    uint64_t ipcp_id;

    ipcp_id = req->ipcp_id();
    object_class = req->object_class();
    object_name = req->object_name();

    if (!IPCManager->ipcp_exists(ipcp_id)) {
        err << "No such IPC process: " << ipcp_id;
        return grpc::Status(grpc::StatusCode::INTERNAL, err.str());
    }

    if (IPCManager->query_rib(this->addon, &promise, ipcp_id, object_class, object_name) == IPCM_FAILURE ||
        promise.wait() != IPCM_SUCCESS)
        return grpc::Status(grpc::StatusCode::INTERNAL, "Query RIB operation failed");

    for (it = promise.rib_objects.begin(); it != promise.rib_objects.end(); ++it) {
        ipcm_api::RIBObject *ro;

        ro = res->add_rib_object();
        ro->set_name((*it).name);
        ro->set_class_((*it).clazz);
        ro->set_instance((*it).instance);
        ro->set_displayable_value((*it).displayable_value);
    }

    return grpc::Status::OK;
}

grpc::Status IPCMApiService::CreateIpcp(grpc::ServerContext* ctx,
                                       const ipcm_api::CreateIpcpReq* req,
                                       ipcm_api::CreateIpcpRes* res) {
    CreateIPCPPromise promise;
    rina::ApplicationProcessNamingInformation ipcp_name;
    std::string ipcp_type, dif_name;

    ipcp_type = req->ipcp_type();
    dif_name = req->dif_name();

    if(IPCManager->create_ipcp(this->addon, &promise, ipcp_name, ipcp_type, dif_name) == IPCM_FAILURE ||
       promise.wait() != IPCM_SUCCESS)
        return grpc::Status(grpc::StatusCode::INTERNAL, "Create IPCP operation failed");

    res->set_name(ipcp_type);
    res->set_ipcp_id(promise.ipcp_id);

    return grpc::Status::OK;
}

grpc::Status IPCMApiService::DestroyIpcp(grpc::ServerContext* ctx,
                                         const ipcm_api::DestroyIpcpReq* req,
                                         ipcm_api::DestroyIpcpRes* res) {
    uint64_t ipcp_id;
    std::ostringstream err;

    ipcp_id = req->ipcp_id();

    if (!IPCManager->ipcp_exists(ipcp_id)) {
        err << "No such IPC process: " << ipcp_id;
        return grpc::Status(grpc::StatusCode::INTERNAL, err.str());
    }

    if (IPCManager->destroy_ipcp(this->addon, ipcp_id) != IPCM_SUCCESS)
        return grpc::Status(grpc::StatusCode::INTERNAL, "Destroy IPCP operation failed");

    return grpc::Status::OK;
}

grpc::Status IPCMApiService::EnrollToDif(grpc::ServerContext* ctx,
                                         const ipcm_api::EnrollToDifReq* req,
                                         ipcm_api::EnrollToDifRes* res) {
    uint64_t ipcp_id;
    int t1, t0 = rina::Time::get_time_in_ms();
    std::ostringstream err;
    Promise promise;
    NeighborData neighbor_data;

    neighbor_data.difName =
        rina::ApplicationProcessNamingInformation(req->dif_name(), std::string());
    neighbor_data.supportingDifName =
        rina::ApplicationProcessNamingInformation(req->supporting_dif_name(), std::string());

    if (req->neighbor_process_name().size() > 0)
        neighbor_data.apName =
            rina::ApplicationProcessNamingInformation(req->neighbor_process_name(), req->neighbor_process_inst());

    if (!IPCManager->ipcp_exists(req->ipcp_id())) {
        err << "No such IPC process: " << req->ipcp_id();
        return grpc::Status(grpc::StatusCode::INTERNAL, err.str());
    }

    if(IPCManager->enroll_to_dif(this->addon, &promise, req->ipcp_id(), neighbor_data) == IPCM_FAILURE ||
       promise.wait() != IPCM_SUCCESS)
        return grpc::Status(grpc::StatusCode::INTERNAL, "Enrollment operation failed");

    t1 = rina::Time::get_time_in_ms();

    res->set_ms_delay(t1 - t0);

    return grpc::Status::OK;
}

grpc::Status IPCMApiService::AssignToDif(grpc::ServerContext* ctx,
                                         const ipcm_api::AssignToDifReq* req,
                                         ipcm_api::AssignToDifRes* res) {
    std::ostringstream err;
    rinad::DIFTemplate dif_template;
    Promise promise;
    int rv;

    rina::ApplicationProcessNamingInformation dif_name(req->dif_name(), std::string());

    if (!IPCManager->ipcp_exists(req->ipcp_id())) {
        err << "No such IPC process: " << req->ipcp_id();
        return grpc::Status(grpc::StatusCode::INTERNAL, err.str());
    }

    rv = IPCManager->dif_template_manager->get_dif_template(req->dif_template(), dif_template);
    if (rv != 0) {
        err << "Cannot find DIF template: " << req->dif_template();
        return grpc::Status(grpc::StatusCode::INTERNAL, err.str());
    }

    if (IPCManager->assign_to_dif(this->addon, &promise, req->ipcp_id(), dif_template, dif_name) == IPCM_FAILURE ||
        promise.wait() != IPCM_SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "DIF assignment failed");
    }

    return grpc::Status::OK;
}

IPCMApiService::IPCMApiService(IPCMApi* addon) {
    this->addon = addon;
}

const std::string IPCMApi::NAME = "grpc";

void IPCMApi::server_loop() {
    IPCMApiService service(this);
	char fill[108] = "";
    unsigned len = socket_path.size();
    struct sockaddr_un addr_remote;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;

    // Register "service" as the instance through which we'll
    // communicate with clients. In this case it corresponds to an
    // *synchronous* service.
    builder.RegisterService(&service);

    // Finally assemble the server.
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    LOG_INFO("GRPC Server listening on %s", socket_path.c_str());

    int addr_remote_size = sizeof(addr_remote);

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    while(true) {
        int client_fd = accept(server_fd, (struct sockaddr *)&addr_remote, (socklen_t*)&addr_remote_size);

        if (client_fd < 0)
            throw std::runtime_error("error accepting on uds");

        /* Make the socket non-blocking. */
        int flags = fcntl(client_fd, F_GETFL, 0);

        if (flags < 0)
            throw std::runtime_error("error getting flags for uds");

        if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0)
            throw std::runtime_error("error setting non-blocking for uds");

        /* Make the GRPC server use the socket. */
        grpc::AddInsecureChannelFromFd(server.get(), client_fd);
    }
}

bool IPCMApi::cleanup_filesystem_socket() {
	struct stat stat;
	const char *path = socket_path.c_str();
	// Ignore if it's not a socket to avoid data loss;
	// bind() will fail and we'll output a log there.
	return lstat(path, &stat) || !S_ISSOCK(stat.st_mode) || !unlink(path);
}

// Service thread entry point.
void* service_function(void *opaque) {
    IPCMApi* service = static_cast<IPCMApi*>(opaque);

    service->server_loop();

    return NULL;
}

bool IPCMApi::init() {
	struct sockaddr_un sa_un;
	char fill[108] = "";

    // Initialize the server's socket.
	memcpy(sa_un.sun_path, fill, 108);
	int sfd = -1;
	unsigned len = socket_path.size();

	if (len >= sizeof sa_un.sun_path) {
		LOG_ERR("AF_UNIX path too long");
		return false;
	}

	socket_path.copy(sa_un.sun_path, len);
	if (socket_path[0] == '@') {
		sa_un.sun_path[0] = 0; // abstract unix socket
	} else if (!cleanup_filesystem_socket()) {
		LOG_ERR("Error [%i] unlinking %s", errno, socket_path.c_str());
        return false;
	}
	len += offsetof(struct sockaddr_un, sun_path);

	sa_un.sun_family = AF_UNIX;
	try {
		server_fd = socket(sa_un.sun_family, SOCK_STREAM, 0);
		if (server_fd < 0)
			throw rina::Exception("socket");
		if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&sa_un), len))
			throw rina::Exception("bind");
		if (listen(server_fd, 5))
			throw rina::Exception("listen");

	} catch (rina::Exception& e) {
		LOG_ERR("Error [%i] calling %s()", errno, e.what());
		if (sfd >= 0) {
			close(server_fd);
			sfd = -1;
            return false;
		}

        if (!cleanup_filesystem_socket()) {
            LOG_ERR("Error [%i] unlinking %s", errno, socket_path.c_str());
            return false;
        }
	}

    return true;
}

IPCMApi::IPCMApi(const std::string& socket_path_)
    : Addon(IPCMApi::NAME),
      socket_path(socket_path_)
{
    if (init()) {
        // Start the service in a new thread.
        worker = new rina::Thread(service_function, this, std::string("grpc-api"), false);
        worker->start();
    }
    else LOG_ERR("Failed initialization of GRPC API server.");
}

IPCMApi::~IPCMApi() {
	void *status;

	worker->join(&status);
	if (socket_path[0] != '@')
		cleanup_filesystem_socket();

	delete worker;
}

} // namespace rinad
