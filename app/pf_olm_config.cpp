/********************************************************************************
 * File Name: pf_olm_config.cpp
 *
 * Version: 1.0
 *
 * Description:
 * This file contains functions related to setting and getting the packet filter
 * configuration.
 *
 *********************************************************************************
 * Copyright (2019), Cypress Semiconductor Corporation. All rights reserved.
 *******************************************************************************
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
 ********************************************************************************/

#include "whd_types.h"
#include "cy_lpa_wifi_ol_common.h"
#include "cy_lpa_wifi_ol.h"
#include "cy_lpa_wifi_pf_ol.h"
#include "string.h"
#include "WhdSTAInterface.h"
#include "WhdOlmInterface.h"
#include "pf_olm_config.h"

/*********************************************************************
 *                           EXTERNS
 ********************************************************************/
extern    WhdSTAInterface* wifi;
extern    void             restart_olm();
extern    int8_t*          get_entry_id();
extern    ol_desc_t        new_olm_list[];
extern    void             app_wl_disconnect(WhdSTAInterface *wifi);
extern    int              app_wl_connect(WhdSTAInterface *wifi,
                                               const char *ssid,
                                               const char *pass,
                                     nsapi_security_t security);

/*********************************************************************
 *                           ENUMS
 ********************************************************************/
enum filter_type {
    PF = 1,
    ET,
    IT
};

/*********************************************************************
 *                       GLOBAL VARIABLES
 ********************************************************************/
/*
 * Need two lists of cfgs since we don't want to alter a list while its
 * in use by olm. So ping pong between the two, filling one while the
 * other is in use.
 */
cy_pf_ol_cfg_t ping_tmp_cfgs[MAX_FILTERS];
cy_pf_ol_cfg_t pong_tmp_cfgs[MAX_FILTERS];

/*
 * Define a set of ping-pong buffers. While the olm has control of a
 * list it can't be changed (since olm is still using it).
 * So while one list is consumed by olm, we will modify the other list.
 *
 * The pf_commit_list() function will run the olm deinit routines and then
 * restart olm with a new list and the old list is free to use.
 */
struct ping_pong_t
{
    cy_pf_ol_cfg_t *first;  /* Start of buffer */
    cy_pf_ol_cfg_t *cur;    /* Current position in buffer */
    cy_pf_ol_cfg_t *last;   /* End of buffer */
} pongbufs[2] = {
    { &ping_tmp_cfgs[0], &ping_tmp_cfgs[0], &ping_tmp_cfgs[MAX_FILTERS - 1] }, /* ping */
    { &pong_tmp_cfgs[0], &pong_tmp_cfgs[0], &pong_tmp_cfgs[MAX_FILTERS - 1] }  /* pong */
};

cy_pf_ol_cfg_t *downloaded = (cy_pf_ol_cfg_t *)((ol_desc_t *)get_default_ol_list())->cfg;

/*********************************************************************
 *                       STATIC VARIABLES
 ********************************************************************/
static int cur_pong_idx = 0; /* Index of the pingpong buf we are currently using */
static struct ping_pong_t *pong = &pongbufs[cur_pong_idx];  /* Pointer to current struct */

/********************************************************************
 *                     FUNCTION DEFINITIONS
 *******************************************************************/
/*******************************************************************************
* Function Name: hash_function
********************************************************************************
*
* Summary:
* This function returns a enum value based on the config string passed.
* Packet Filter(PF) = 1; Eth Type(ET) = 2; IP Type(IT) = 3.
*
* Parameters:
* char *str: A string value from HTTP data.
*
* Return:
* int: Returns value between 1 and 3 if the HTTP config matches with any enum
* entry. Otherwise, it returns 0.
*
*******************************************************************************/
static int hash_function(char *str)
{
    if (!strncmp(str, "PF", 2)) return PF;
    if (!strncmp(str, "ET", 2)) return ET;
    if (!strncmp(str, "IT", 2)) return IT;

    return 0;
}

/*******************************************************************************
* Function Name: ping_pong
********************************************************************************
*
* Summary:
* This function uses a ping pong buffer. We used two buffers of type cy_pf_ol_cfg_t
* and each with buffer size of MAX_FILTERS. One buffer is used to hold the actively
* running packet filter configuration and the other buffer is used to accumulate
* the pending packet filters. This function is basically used to swap the active
* buffer index whenever new configuration has applied.
*
* Parameters:
* None.
*
* Return:
* void.
*
*******************************************************************************/
static void ping_pong()
{
    downloaded = pong->first;

    /* Switch to other buffer */
    cur_pong_idx = !cur_pong_idx;
    pong = &pongbufs[cur_pong_idx];

    /* Clear out new buffer and reset cur pointer to begining */
    memset(pong->first, 0, sizeof(ping_tmp_cfgs) );
    pong->cur = pong->first;

}

/*******************************************************************************
* Function Name: check_filter_type
********************************************************************************
*
* Summary:
* This function checks an error scenario. As per the application design, only Keep
* or only Discard packet filters are allowed. The combination of Keep and Discard
* filter does not make any sense and the filter will not get added to the pending
* list. Also, adding multiple discard filter is not supported. The number of discard
* filters that can be added to pending list is limited to 1. This is a retriction
* from WLAN firmware as per its design.
*
* Parameters:
* char *config_str[]: Pointer to filter data. The filter data comes from HTTP
* web page as a string and this variable is used to hold pointer to it.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR indicating whether
* the filter is a valid one or not.
*******************************************************************************/
static cy_rslt_t check_filter_type(char *config_str[])
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* 
     * Only one Discard packet filter is supported by the WLAN.
     * So check here for discard filter already present. If yes,
     * return ERROR since no more configuration is allowed
     * further unless the discard filter is removed from the
     * pending list.
     */
    if (pong->first->bits & CY_PF_ACTION_DISCARD)
    {
        MBED_APP_ERR(("No further configuration is allowed since the list contains a discard filter. "
                      "Note that only one discard packet filter is allowed\n"));
        return CY_RSLT_TYPE_ERROR;
    }

    /* Allow only Keep or only Discard packet filter */
    if (pong->first->feature != 0 &&
        pong->first->feature != CY_PF_OL_FEAT_LAST)
    {
        if (!strncmp(config_str[1], "K", 1))
        {
            if (pong->first->bits & CY_PF_ACTION_DISCARD)
            {
                result = CY_RSLT_TYPE_ERROR;
            }
        } else {
            if (!(pong->first->bits & CY_PF_ACTION_DISCARD))
            {
                result = CY_RSLT_TYPE_ERROR;
            }
        }
    }

    return result;
}

/*******************************************************************************
* Function Name: validate_filter_before_add
********************************************************************************
*
* Summary:
* This function is used to check if the new filter about to get added in the pending
* filter list differs from all the existing filters present in the pending list.
* This ensures uniqueness of the filter getting added to the pending filter list
* and hence avoids duplicate filters in the list.
*
* Parameters:
* char *config_str[]: Pointer to filter data. The filter data comes from HTTP
* web page as a string and this variable is used to hold pointer to it.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR indicating whether
* the filter is not a duplicate one.
*******************************************************************************/
static cy_rslt_t validate_filter_before_add(char *config_str[])
{
    int already_exists = 0;
    cy_rslt_t result   = CY_RSLT_SUCCESS;

    /* Check for unique filter action type - only Keep or only Discard */
    result = check_filter_type(config_str);

    if (result != CY_RSLT_SUCCESS) {
        MBED_APP_ERR(("Either only keep or only discard filters are allowed\n"));
        return result;
    }

    /* Check if the filter already exists */
    for (cy_pf_ol_cfg_t *cfg = pong->first; cfg < pong->cur; cfg++)
    {
        if (cfg->feature != hash_function(config_str[0]))
        {
            continue;
        }
        switch (cfg->feature)
        {
            case CY_PF_OL_FEAT_PORTNUM:
              if (cfg->u.pf.portnum.portnum != atoi(config_str[4]))
              {
                  continue;
              }
              else
              {
                  if (!strncmp(config_str[3], "SP", 2) &&
                      (cfg->u.pf.portnum.direction != PF_PN_PORT_SOURCE))
                  {
                      continue;
                  } else if (!strncmp(config_str[3], "DP", 2) &&
                             (cfg->u.pf.portnum.direction != PF_PN_PORT_DEST))
                  {
                      continue;
                  }

                  if (!strncmp(config_str[2], "T", 1) &&
                      (cfg->u.pf.proto != CY_PF_PROTOCOL_TCP))
                  {
                      continue;
                  } else if (!strncmp(config_str[2], "U", 1) &&
                             (cfg->u.pf.proto != CY_PF_PROTOCOL_UDP))
                  {
                      continue;
                  }
              }
              break;
            case CY_PF_OL_FEAT_ETHTYPE:
              if (pong->cur->u.eth.eth_type != (uint16_t)strtol(config_str[5], NULL, 16))
              {
                  continue;
              }
              break;
            case CY_PF_OL_FEAT_IPTYPE:
              if (pong->cur->u.ip.ip_type != (uint8_t)atoi(config_str[6]))
              {
                  continue;
              }
              break;
            default:
              MBED_APP_ERR(("Unsupported feature type: %d\n", cfg->feature));
              break;
        }

        /* Found duplicate entry */
        already_exists = 1;
        break;
    }

    if (already_exists) {
        result = CY_RSLT_TYPE_ERROR;
    }

    return result;
}

/*****************************************************************************************
* Function Name: get_max_filter
******************************************************************************************
*
* Summary:
* This function returns integer (MAX_FILTERS-1). The last entry in packet filter configuration
* is reserved for a NULL entry. This function is used to check if maximum entry limit has
* reached before adding new entry.
*
* Parameters:
* None.
*
* Return:
* uint16_t: Returns (MAX_FILTERS-1).
*
*****************************************************************************************/
uint16_t get_max_filter()
{
    return (MAX_FILTERS-1);
}

/*****************************************************************************************
* Function Name: get_pending_filter_list
******************************************************************************************
*
* Summary:
* This function returns the pointer to pending packet filter list.
*
* Parameters:
* None.
*
* Return:
* cy_pf_ol_cfg_t*: Pointer to pending packet filter configuration.
*
*****************************************************************************************/
cy_pf_ol_cfg_t *get_pending_filter_list()
{
    return pong->first;
}

/*****************************************************************************************
* Function Name: add_minimum_filters
******************************************************************************************
*
* Summary:
* This function imports default minimum packet filter configuration to the pending packet
* filter list. It will get applied to the WLAN device only after clicking Apply Filters
* web button. The default minimum packet filters are taken from the generated configuration
* as selected on the device-configurator.
*
* Parameters:
* None.
*
* Return:
* void.
*
*****************************************************************************************/
void add_minimum_filters()
{
    int8_t tmp_id = 0;
    int8_t *entry_id;

    cy_pf_ol_cfg_t *default_filters = (cy_pf_ol_cfg_t *)((ol_desc_t *)get_default_ol_list())->cfg;

    /* Copy default packet filters */
    memcpy(pong->first, default_filters, sizeof(pong_tmp_cfgs));

    for (pong->cur = pong->first; pong->cur->feature != CY_PF_OL_FEAT_LAST; pong->cur++)
    {
         tmp_id = pong->cur->id+1;
    }

    entry_id = get_entry_id();
    *entry_id = tmp_id;
}

/*****************************************************************************************
* Function Name: remove_last_added_filter
******************************************************************************************
*
* Summary:
* This function deletes the last added filter from the pending packet filter list.
* Note that, the pending packet filter list is a draft list and it will get applied to the
* WLAN device only when Apply Filters option is clicked.
*
* Parameters:
* None.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR indicating whether the entry
* has been removed successfully or not.
*
*****************************************************************************************/
cy_rslt_t remove_last_added_filter()
{
    cy_pf_ol_cfg_t *cfg = NULL;
    cy_pf_ol_cfg_t *filter_to_remove = NULL;
    int8_t *entry_id;

    /* Check for empty and proceed */
    if (pong->first->feature == 0 || pong->first->feature == CY_PF_OL_FEAT_LAST)
    {
        MBED_APP_ERR(("Attempt to remove NULL entry. Exit.\n"));
        return CY_RSLT_TYPE_ERROR;
    }

    /* Go to last entry to remove */
    for (cfg = pong->first; cfg->feature != CY_PF_OL_FEAT_LAST; cfg++)
    {
        filter_to_remove = cfg;
    }

    /* Mark the entry with CY_PF_OL_FEAT_LAST flag */
    memset(filter_to_remove, 0, sizeof(cy_pf_ol_cfg_t)*2);
    pong->cur = filter_to_remove;
    pong->cur->feature = CY_PF_OL_FEAT_LAST;

    /* Decrement entry id to be created next as we removed one */
    entry_id = get_entry_id();
    if (*entry_id)
    {
        *entry_id = *entry_id - 1;
    }

    return CY_RSLT_SUCCESS;
}

/*****************************************************************************************
* Function Name: print_filter
******************************************************************************************
*
* Summary:
* This function builds a string output from the packet filter configuration in user
* readable way. The string output can be directly printed out.
*
* Parameters:
* cy_pf_ol_cfg_t* cfg: Pointer to packet filter configuration.
* char* http_str_builder: Pointer to HTTP response string to be sent to the server.
* char* build_str: Helper variable to Buffer for storing the formatted string from this function.
* int build_str_len: Buffer size.
*
* Return:
* void.
*
*****************************************************************************************/
void print_filter(cy_pf_ol_cfg_t *cfg, char *http_str_builder, char *build_str, int build_str_len)
{
    if (!cfg->feature) {
        return;
    }

    sprintf(build_str, "ID %d", cfg->id);
    strcat(http_str_builder, build_str);
    switch (cfg->feature)
    {
        case CY_PF_OL_FEAT_PORTNUM:
            strcat(http_str_builder, "[Port Filter]:\n");
            memset(build_str, '\0', build_str_len);
            sprintf(build_str, "\tPort = %d,\n", cfg->u.pf.portnum.portnum);
            strcat(http_str_builder, build_str);

            /* Packet filter type - Keep or Discard packet */
            if (cfg->bits & CY_PF_ACTION_DISCARD)
                strcat(http_str_builder, "\tAction = Discard,\n");
            else
                strcat(http_str_builder, "\tAction = Keep,\n");

            /* Protocol TCP or UDP */
            switch (cfg->u.pf.proto)
            {
                case CY_PF_PROTOCOL_TCP:
                    strcat(http_str_builder, "\tProtocol = TCP,\n");
                    break;
                case CY_PF_PROTOCOL_UDP:
                    strcat(http_str_builder, "\tProtocol = UDP,\n");
                    break;
                default:
                    MBED_APP_ERR(("Unknown IP Protocol used with Port Numbers: %d\n", cfg->u.pf.proto));
                    break;
            }

            /* Packet direction - Source or Destination port */
            if (cfg->u.pf.portnum.direction == PF_PN_PORT_SOURCE)
                strcat(http_str_builder, "\tDirection = Source Port\n");
            else
                strcat(http_str_builder, "\tDirection = Destination Port\n");

            break;

        case CY_PF_OL_FEAT_ETHTYPE:
            strcat(http_str_builder, "[Eth Type]:\n");
            memset(build_str, '\0', build_str_len);
            sprintf(build_str, "\tPacket Type = 0x%x,\n", cfg->u.eth.eth_type);
            strcat(http_str_builder, build_str);

            /* Packet filter type - Keep or Discard packet */
            if (cfg->bits & CY_PF_ACTION_DISCARD)
                strcat(http_str_builder, "\tAction = Discard\n");
            else
                strcat(http_str_builder, "\tAction = Keep\n");

            break;

        case CY_PF_OL_FEAT_IPTYPE:
            strcat(http_str_builder, "[IP Type]:\n");
            memset(build_str, '\0', build_str_len);
            sprintf(build_str, "\tPacket Type = 0x%x,\n", cfg->u.ip.ip_type);
            strcat(http_str_builder, build_str);

            /* Packet filter type - Keep or Discard packet */
            if (cfg->bits & CY_PF_ACTION_DISCARD)
                strcat(http_str_builder, "\tAction = Discard\n");
            else
                strcat(http_str_builder, "\tAction = Keep\n");

            break;
        default:
            MBED_APP_ERR(("Unknown feature: %d\n", cfg->feature));
            break;
    }
    strcat(http_str_builder, "\n");
}

/*****************************************************************************************
* Function Name: pf_add_to_list
******************************************************************************************
*
* Summary:
* This function adds a new packet filter configuration from HTTP server to the
* pending filter list. It checks for valid packet filter and adds to the list
* only if all the conditions are satisfied.
*
* Parameters:
* char*   config_str[]: Pointer to filter data. The filter data comes from HTTP
* server as a string and this variable is used to hold pointer to it.
* uint8_t id: This is the index number for that particular packet filter entry.
* Each entry in the packet filter list has unique ID assigned.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR.
*
*****************************************************************************************/
cy_rslt_t pf_add_to_list(char *config_str[], uint8_t id)
{
    /* Verify there is room left for the FEATURE_LAST at the end */
    if (pong->cur >= pong->last)
    {
        MBED_APP_ERR(("Max number of entries %d.\n", MAX_FILTERS - 1));
        return CY_RSLT_TYPE_ERROR;
    }

   /* Validate filter before adding to pending list. */
    if (CY_RSLT_SUCCESS != validate_filter_before_add(config_str))
    {
        return CY_RSLT_TYPE_ERROR;
    }

    pong->cur->id = id;
    /* Setup minimum required defaults */
    for (cy_pf_ol_cfg_t *cfg = pong->first; cfg < pong->cur; cfg++)
    {
        if (cfg->id == id)
        {
            MBED_APP_ERR(("Filter id %d already in pending list\n", cfg->id));
            pong->cur->id = pong->cur->id+1;
            break;
        }
    }

    pong->cur->bits |= CY_PF_ACTIVE_SLEEP;
    pong->cur->bits |= CY_PF_ACTIVE_WAKE;

    switch(hash_function(config_str[0]))
    {
        case PF: //Port Filter
             uint32_t port_num;
             pong->cur->feature = CY_PF_OL_FEAT_PORTNUM;
             pong->cur->u.pf.portnum.range = 0;
             port_num = atoi(config_str[4]);

             if (port_num > 65535)
             {
                 MBED_APP_ERR(("Invalid port number (%ld). Valid range is 0-65535.\n",
                              port_num));
                 memset(pong->cur, 0, sizeof(cy_pf_ol_cfg_t));
                 return CY_RSLT_TYPE_ERROR;
             }

             pong->cur->u.pf.portnum.portnum = (port_num & 0xFFFF);

             //Keep or Discard packet
             if (!strncmp(config_str[1], "K", 1))
             {
                 pong->cur->bits &= ~CY_PF_ACTION_DISCARD;
             } else {
                 pong->cur->bits |= CY_PF_ACTION_DISCARD;
             }
             //Source or Destination port
             if (!strncmp(config_str[3], "SP", 2))
             {
                 pong->cur->u.pf.portnum.direction = PF_PN_PORT_SOURCE;
             } else {
                 pong->cur->u.pf.portnum.direction = PF_PN_PORT_DEST;
             }
             //Protocol type
             if (!strncmp(config_str[2], "T", 1))
             {
                 pong->cur->u.pf.proto = CY_PF_PROTOCOL_TCP;
             } else {
                 pong->cur->u.pf.proto = CY_PF_PROTOCOL_UDP;
             }
             break;
        case ET: //Ether type Filter
             uint32_t eth_type;
             pong->cur->feature = CY_PF_OL_FEAT_ETHTYPE;
             eth_type = strtol(config_str[5], NULL, 0);

             if (!eth_type || (eth_type & ~0xFFFF) || (eth_type < 0x800) ) {
                 MBED_APP_ERR(("Invalid eth type (0x%lx).  Range is 0x800 - 0xFFFF\n", eth_type));
                 memset(pong->cur, 0, sizeof(cy_pf_ol_cfg_t));
                 return CY_RSLT_TYPE_ERROR;
             }
             pong->cur->u.eth.eth_type = (eth_type & 0xFFFF);

             //Keep or Discard packet
             if (!strncmp(config_str[1], "K", 1))
             {
                 pong->cur->bits &= ~CY_PF_ACTION_DISCARD;
             } else {
                 pong->cur->bits |= CY_PF_ACTION_DISCARD;
             }
             break;
        case IT: //IP type filter
             uint32_t ip_proto;
             pong->cur->feature = CY_PF_OL_FEAT_IPTYPE;
             ip_proto = strtoul(config_str[6], NULL, 0);

             if (!ip_proto || ip_proto & ~0xFF) {
                 MBED_APP_ERR(("Invalid IP protocol (0x%lx).  Range is 1 - 255\n", ip_proto));
                 memset(pong->cur, 0, sizeof(cy_pf_ol_cfg_t));
                 return CY_RSLT_TYPE_ERROR;
             }
             pong->cur->u.ip.ip_type = (ip_proto & 0xFF);

             //Keep or Discard packet
             if (!strncmp(config_str[1], "K", 1))
             {
                 pong->cur->bits &= ~CY_PF_ACTION_DISCARD;
             } else {
                 pong->cur->bits |= CY_PF_ACTION_DISCARD;
             }
             break;
        default:
             MBED_APP_ERR(("Unknown Packet Filter Type received\n"));
             break;
    }

    pong->cur++;
    pong->cur->feature = CY_PF_OL_FEAT_LAST;

    return CY_RSLT_SUCCESS;
}

/*****************************************************************************************
* Function Name: pf_commit_list
******************************************************************************************
*
* Summary:
* This function applies the pending packet filter list to the WLAN device.
* The WLAN device will disconnect and reconnect to the AP in order for the new packet
* filter to become active.
*
* Parameters:
* bool restore_to_default: If TRUE, it will restore the default packet filter configuration
* as selected in device configurator. If FALSE, it will apply the new packet filter
* configuration to the WLAN device.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR.
*
*****************************************************************************************/
cy_rslt_t pf_commit_list(bool restore_to_default)
{
    nsapi_error_t nsapi_err;

    MBED_APP_INFO(("Applying new packet filter list\n"));

    if (pong->cur > pong->last)
    {
        MBED_APP_ERR(("List is full.\n"));
        return CY_RSLT_TYPE_ERROR;
    }

    /* Terminate list with FEAT_LAST */
    pong->cur->feature = CY_PF_OL_FEAT_LAST;

    /* Wifi disconnect is how we cause an offload deinit. */
    app_wl_disconnect(wifi);

    /* Switch to fresh new buffer so we don't disturb the olm controlled buffer */
    ping_pong();

    /* Restore to default packet filter configuration defined
     * by the device configurator
     */
    if (restore_to_default) {
        downloaded = (cy_pf_ol_cfg_t *)((ol_desc_t *)get_default_ol_list())->cfg;
    }

    /* Restart OLM to use new packet filter configs */
    restart_olm();

    /* Reassociate to AP */
    MBED_APP_INFO(("Re-Associate\n"));
    nsapi_err = app_wl_connect(wifi, MBED_CONF_APP_WIFI_SSID, 
                                 MBED_CONF_APP_WIFI_PASSWORD,
                                MBED_CONF_APP_WIFI_SECURITY);
    if (nsapi_err != NSAPI_ERROR_OK)
    {
        MBED_APP_ERR(("Assocation Failed: %d\n", nsapi_err));
        return CY_RSLT_TYPE_ERROR;
    }

    return CY_RSLT_SUCCESS;
}

