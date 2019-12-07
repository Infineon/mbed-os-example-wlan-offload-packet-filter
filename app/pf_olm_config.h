/********************************************************************************
 * File Name: pf_olm_config.h
 *
 * Version: 1.0
 *
 * Description:
 *   This is the header file and contains macro definition and function declarations
 *   for the functions defined in pf_olm_config.cpp file.
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

#ifndef PF_OLM_CONFIG_H
#define PF_OLM_CONFIG_H

#include "cy_lpa_wifi_pf_ol.h"

/*********************************************************************
 *                           MACROS
 ********************************************************************/
/*
 * Define max filters implemented by test app.
 * This does not reflect how many filters the dongle/FW can support.
 * Room for MAX_FILTERS-1 filters plus 1 Null filter at the end of struct.
 */
#define MAX_FILTERS        (11)
#define MBED_APP_INFO(x)   printf x
#define MBED_APP_ERR(x)    printf x

/********************************************************************
 *                     FUNCTION DECLARATIONS
 *******************************************************************/
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
cy_rslt_t       pf_add_to_list(char* config_str[], uint8_t id);

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
cy_rslt_t       pf_commit_list(bool restore_to_default);

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
void            print_filter(cy_pf_ol_cfg_t* cfg,
                             char* http_str_builder,
                             char* build_str,
                             int build_str_len);

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
cy_pf_ol_cfg_t* get_pending_filter_list();

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
uint16_t        get_max_filter();

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
void            add_minimum_filters();

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
cy_rslt_t       remove_last_added_filter();

/*******************************************************************************
* Function Name: app_wl_disconnect
********************************************************************************
*
* Summary:
* This function disconnects the WiFi of the kit from the AP.
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
*
* Return:
* void.
*
*******************************************************************************/
void           app_wl_disconnect(WhdSTAInterface *wifi);

#endif //PF_OLM_CONFIG_H
