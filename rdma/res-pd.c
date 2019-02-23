// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/*
 * res-pd.c	RDMA tool
 * Authors:     Leon Romanovsky <leonro@mellanox.com>
 */

#include "res.h"
#include <inttypes.h>

int res_pd_parse_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[RDMA_NLDEV_ATTR_MAX] = {};
	struct nlattr *nla_table, *nla_entry;
	struct rd *rd = data;
	const char *name;
	uint32_t idx;

	mnl_attr_parse(nlh, 0, rd_attr_cb, tb);
	if (!tb[RDMA_NLDEV_ATTR_DEV_INDEX] || !tb[RDMA_NLDEV_ATTR_DEV_NAME] ||
	    !tb[RDMA_NLDEV_ATTR_RES_PD])
		return MNL_CB_ERROR;

	name = mnl_attr_get_str(tb[RDMA_NLDEV_ATTR_DEV_NAME]);
	idx = mnl_attr_get_u32(tb[RDMA_NLDEV_ATTR_DEV_INDEX]);
	nla_table = tb[RDMA_NLDEV_ATTR_RES_PD];

	mnl_attr_for_each_nested(nla_entry, nla_table) {
		uint32_t local_dma_lkey = 0, unsafe_global_rkey = 0;
		struct nlattr *nla_line[RDMA_NLDEV_ATTR_MAX] = {};
		char *comm = NULL;
		uint32_t ctxn = 0;
		uint32_t pid = 0;
		uint32_t pdn = 0;
		uint64_t users;
		int err;

		err = mnl_attr_parse_nested(nla_entry, rd_attr_cb, nla_line);
		if (err != MNL_CB_OK)
			return MNL_CB_ERROR;

		if (!nla_line[RDMA_NLDEV_ATTR_RES_USECNT] ||
		    (!nla_line[RDMA_NLDEV_ATTR_RES_PID] &&
		     !nla_line[RDMA_NLDEV_ATTR_RES_KERN_NAME])) {
			return MNL_CB_ERROR;
		}

		if (nla_line[RDMA_NLDEV_ATTR_RES_LOCAL_DMA_LKEY])
			local_dma_lkey = mnl_attr_get_u32(
				nla_line[RDMA_NLDEV_ATTR_RES_LOCAL_DMA_LKEY]);

		users = mnl_attr_get_u64(nla_line[RDMA_NLDEV_ATTR_RES_USECNT]);
		if (rd_check_is_filtered(rd, "users", users))
			continue;

		if (nla_line[RDMA_NLDEV_ATTR_RES_UNSAFE_GLOBAL_RKEY])
			unsafe_global_rkey = mnl_attr_get_u32(
				nla_line[RDMA_NLDEV_ATTR_RES_UNSAFE_GLOBAL_RKEY]);

		if (nla_line[RDMA_NLDEV_ATTR_RES_PID]) {
			pid = mnl_attr_get_u32(
				nla_line[RDMA_NLDEV_ATTR_RES_PID]);
			comm = get_task_name(pid);
		}

		if (rd_check_is_filtered(rd, "pid", pid))
			continue;

		if (nla_line[RDMA_NLDEV_ATTR_RES_CTXN])
			ctxn = mnl_attr_get_u32(
				nla_line[RDMA_NLDEV_ATTR_RES_CTXN]);

		if (rd_check_is_filtered(rd, "ctxn", ctxn))
			continue;

		if (nla_line[RDMA_NLDEV_ATTR_RES_PDN])
			pdn = mnl_attr_get_u32(
				nla_line[RDMA_NLDEV_ATTR_RES_PDN]);
		if (rd_check_is_filtered(rd, "pdn", pdn))
			continue;

		if (nla_line[RDMA_NLDEV_ATTR_RES_KERN_NAME])
			/* discard const from mnl_attr_get_str */
			comm = (char *)mnl_attr_get_str(
				nla_line[RDMA_NLDEV_ATTR_RES_KERN_NAME]);

		if (rd->json_output)
			jsonw_start_array(rd->jw);

		print_dev(rd, idx, name);
		if (nla_line[RDMA_NLDEV_ATTR_RES_LOCAL_DMA_LKEY])
			print_key(rd, "local_dma_lkey", local_dma_lkey);
		print_users(rd, users);
		if (nla_line[RDMA_NLDEV_ATTR_RES_UNSAFE_GLOBAL_RKEY])
			print_key(rd, "unsafe_global_rkey", unsafe_global_rkey);
		print_pid(rd, pid);
		print_comm(rd, comm, nla_line);
		if (nla_line[RDMA_NLDEV_ATTR_RES_CTXN])
			res_print_uint(rd, "ctxn", ctxn);

		if (nla_line[RDMA_NLDEV_ATTR_RES_PDN])
			res_print_uint(rd, "pdn", pdn);

		if (nla_line[RDMA_NLDEV_ATTR_RES_PID])
			free(comm);

		print_driver_table(rd, nla_line[RDMA_NLDEV_ATTR_DRIVER]);
		newline(rd);
	}
	return MNL_CB_OK;
}
