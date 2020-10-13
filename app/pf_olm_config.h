/******************************************************************************
 * File Name: pf_olm_config.h
 *
 * Description:
 *   This header file contains macros and function declarations to add/update/
 *   remove the WLAN packet filters.
 * 
 ******************************************************************************
 * Copyright (2019-2020), Cypress Semiconductor Corporation. All rights reserved.
 ******************************************************************************
 * This software, including source code, documentation and related materials
 * (“Software”), is owned by Cypress Semiconductor Corporation or one of its
 * subsidiaries (“Cypress”) and is protected by and subject to worldwide patent
 * protection (United States and foreign), United States copyright laws and
 * international treaty provisions. Therefore, you may use this Software only
 * as provided in the license agreement accompanying the software package from
 * which you obtained this Software (“EULA”).
 *
 * If no EULA applies, Cypress hereby grants you a personal, nonexclusive,
 * non-transferable license to copy, modify, and compile the Software source
 * code solely for use in connection with Cypress’s integrated circuit products.
 * Any reproduction, modification, translation, compilation, or representation
 * of this Software except as specified above is prohibited without the express
 * written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death (“High Risk Product”). By
 * including Cypress’s product in a High Risk Product, the manufacturer of such
 * system or application assumes all risk of such use and in doing so agrees to
 * indemnify Cypress against all liability.
 *****************************************************************************/

#ifndef PF_OLM_CONFIG_H
#define PF_OLM_CONFIG_H

#include "cy_lpa_wifi_pf_ol.h"

/******************************************************************************
 *                                 MACROS
 *****************************************************************************/
/*
 * This application sets the maximum allowed packet filters to 10
 * (MAX_FILTERS-1). The maximum number of filters is ultimately dictated
 * by the free memory available on the Wi-Fi chipset. More memory will allow
 * more filters.
 */
#define MAX_FILTERS                        (11)

/* Packet filter ID length. */
#define PKT_FILTER_ID_STR_LEN              (2)

/* HTTP data buffer index for packet filter types identity. */
#define PKT_FILTER_TYPE_ID                 (0)
#define PKT_FILTER_TYPE_ID_LEN             (2)

/* HTTP data buffer index for Keep or Discard packet identity. */
#define KEEP_OR_DISCARD_ID                 (1)
#define KEEP_OR_DISCARD_ID_LEN             (1)

/* HTTP data buffer for TCP or UDP packet identity. */
#define TCP_OR_UDP_ID                      (2)
#define TCP_OR_UDP_ID_LEN                  (1)

/* HTTP data buffer index for Source port or Destination port identity. */
#define SOURCE_OR_DESTINATION_ID           (3)
#define SOURCE_OR_DESTINATION_ID_LEN       (2)

/* HTTP data buffer index for Port number identity. */
#define PORT_NUMBER_ID                     (4)

/* HTTP data buffer index for Eth type value. */
#define ETH_TYPE_VALUE_ID                  (5)

/* HTTP data buffer index for IP type value. */
#define IP_TYPE_VALUE_ID                   (6)

/* Maximum value of TCP or UDP port number. */
#define MAX_PORT_NUM                       (65535)

/* Maximum number of HTTP user data in the query string. */
#define MAX_HTTP_CONFIG_NUMBER             (7)

/******************************************************************************
 *                     FUNCTION DECLARATIONS
 *****************************************************************************/
cy_pf_ol_cfg_t *get_pending_filter_list(void);
cy_rslt_t pf_add_to_list(char* config_str[], uint8_t id);
cy_rslt_t pf_commit_list(bool restore_to_default);
cy_rslt_t remove_last_added_filter(void);
uint16_t get_max_filter(void);
void add_minimum_filters(void);
void app_wl_disconnect(WhdSTAInterface *wifi);
void print_filter(cy_pf_ol_cfg_t* cfg,
                  char* http_str_builder,
                  char* build_str,
                  int build_str_len);

#endif /* #ifndef PF_OLM_CONFIG_H */


/* [] END OF FILE */

