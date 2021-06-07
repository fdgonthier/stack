/*
 * IPC Manager GRPC interface
 *
 * François-Denis Gonthier <francois-denis@trianetworksystems.com>
 *
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

#ifndef __IPCM_GRPC_API_H__
#define __IPCM_GRPC_API_H__

#include "rina-configuration.h"
#include "../addon.h"

#include "proto/ipcm_api.grpc.pb.h"

namespace rinad {

class IPCMApi : public Addon {
    private:
    std::string socket_path;

    bool cleanup_filesystem_socket();

    rina::Thread* worker;

    /**
     * UNIX socket file descriptor.
     */
    int server_fd;

    bool init();

public:
	static const std::string NAME;

    void server_loop();

	IPCMApi(const std::string&);
	virtual ~IPCMApi() throw();

	int plugin_load_unload(std::vector<std::string>&, bool);
};

class IPCMApiService final : public ipcm_api::IPCM::Service {
    private:
    IPCMApi* addon;

    public:
    grpc::Status ListIpcps(grpc::ServerContext*,
                           const ipcm_api::ListIpcpsReq*,
                           ipcm_api::ListIpcpsRes*) override;

    grpc::Status GetIfaceForDif(grpc::ServerContext*,
                                const ipcm_api::GetIfaceForDifReq*,
                                ipcm_api::GetIfaceForDifRes*) override;

    grpc::Status QueryRib(grpc::ServerContext*,
                          const ipcm_api::QueryRibReq*,
                          ipcm_api::QueryRibRes*) override;

    grpc::Status CreateIpcp(grpc::ServerContext*,
                            const ipcm_api::CreateIpcpReq*,
                            ipcm_api::CreateIpcpRes*) override;

    grpc::Status DestroyIpcp(grpc::ServerContext*,
                             const ipcm_api::DestroyIpcpReq*,
                             ipcm_api::DestroyIpcpRes*) override;

    grpc::Status EnrollToDif(grpc::ServerContext*,
                             const ipcm_api::EnrollToDifReq*,
                             ipcm_api::EnrollToDifRes*) override;

    grpc::Status AssignToDif(grpc::ServerContext*,
                             const ipcm_api::AssignToDifReq*,
                             ipcm_api::AssignToDifRes*) override;

    IPCMApiService(IPCMApi* addon);
};

}
#endif  /* __IPCM_GRPC_API_H__ */
