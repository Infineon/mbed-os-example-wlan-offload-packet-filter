/*******************************************************************************
* File Name: cycfg_connectivity_wifi.c
*
* Description:
* Connectivity Wi-Fi configuration
* This file was automatically generated and should not be modified.
* Device Configurator: 2.0.0.1483
* Device Support Library (../../../psoc6pdl): 1.3.1.1499
*
********************************************************************************
* Copyright 2017-2019 Cypress Semiconductor Corporation
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
********************************************************************************/

#include "cycfg_connectivity_wifi.h"

#define CYCFG_PF_OL_ENABLED (1u)

static pf_ol_t pf_ol_0;
static const cy_pf_ol_cfg_t cy_pf_ol_cfg_0[] = 
{
	[0u] = {.feature = CY_PF_OL_FEAT_ETHTYPE,
			.bits = CY_PF_ACTIVE_SLEEP | CY_PF_ACTIVE_WAKE,
			.id = 0u,
			.u = {
				.eth = {
						.eth_type = 0x806u,
						},
				},
			},
	[1u] = {.feature = CY_PF_OL_FEAT_ETHTYPE,
			.bits = CY_PF_ACTIVE_SLEEP | CY_PF_ACTIVE_WAKE,
			.id = 1u,
			.u = {
				.eth = {
						.eth_type = 0x888Eu,
						},
				},
			},
	[2u] = {.feature = CY_PF_OL_FEAT_PORTNUM,
			.bits = CY_PF_ACTIVE_SLEEP | CY_PF_ACTIVE_WAKE,
			.id = 2u,
			.u = {
				.pf = {
						.portnum = {.portnum = 68u,
									.range = 0u,
									.direction = PF_PN_PORT_DEST,},
						.proto = CY_PF_PROTOCOL_UDP,
					},
				},
			},
	[3u] = {.feature = CY_PF_OL_FEAT_PORTNUM,
			.bits = CY_PF_ACTIVE_SLEEP | CY_PF_ACTIVE_WAKE,
			.id = 3u,
			.u = {
				.pf = {
						.portnum = {.portnum = 53u,
									.range = 0u,
									.direction = PF_PN_PORT_SOURCE,},
						.proto = CY_PF_PROTOCOL_UDP,
					},
				},
			},
	[4u] = {.feature = CY_PF_OL_FEAT_PORTNUM,
			.bits = CY_PF_ACTIVE_SLEEP | CY_PF_ACTIVE_WAKE,
			.id = 4u,
			.u = {
				.pf = {
						.portnum = {.portnum = 80u,
									.range = 0u,
									.direction = PF_PN_PORT_DEST,},
						.proto = CY_PF_PROTOCOL_TCP,
					},
				},
			},
	[5u] = {.feature = CY_PF_OL_FEAT_LAST},
};
static const ol_desc_t ol_list_0[] = 
{
	[0u] = {"Pkt_Filter", &cy_pf_ol_cfg_0, &pf_ol_fns, &pf_ol_0},
	[1u] = {NULL, NULL, NULL, NULL},
};

const ol_desc_t *cycfg_get_default_ol_list(void)
{
	return &ol_list_0[0];
}

