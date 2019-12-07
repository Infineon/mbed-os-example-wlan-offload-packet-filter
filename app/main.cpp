/********************************************************************************
 * File Name: main.cpp
 *
 * Version: 1.0
 *
 * Description:
 *   This application demonstrates WLAN Packet Filter offload functionality
 *   supported by Cypress's WiFi chip. The packet filters allows the host
 *   processor to limit what types of packet get passed up to the host from the
 *   WLAN subsystem. So the Host gets greater chance of staying in deep-sleep for
 *   longer time and wake up only when the intended WLAN packet arrives. 
 *   It uses MTB2.0 device-configurator tool to configure default packet
 *   filters needed for a basic HTTP network application to run.
 *   This application hosts a HTTP webserver that allows the user to modify
 *   the default applied packet filters and apply them on the fly.
 *
 * Related Document: README.md
 *                   AN227910 Low-Power System Design with CYW43012 and PSoC 6
 *
 * Supported Kits (Target Names):
 *   CY8CKIT-062-WiFi-BT PSoC 6 WiFi-BT Pioneer Kit (CY8CKIT_062_WIFI_BT)
 *   CY8CPROTO-062-4343W PSoC 6 Wi-Fi BT Prototyping Kit (CY8CPROTO_062_4343W)
 *   CY8CKIT_062S2_43012 PSoC 6 WiFi-BT Pioneer Kit (CY8CKIT_062S2_43012)
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
 *******************************************************************************/

#include "mbed.h"
#include "cy_lpa_wifi_ol.h"
#include "WhdSTAInterface.h"
#include "WhdOlmInterface.h"
#include "cy_lpa_wifi_ol_common.h"
#include "HTTP_server.h"
#include "pf_olm_config.h"
#include "network_activity_handler.h"

/*********************************************************************
 *                           MACROS
 ********************************************************************/
/* Response string buffer to be sent to HTTP stream */
#define HTTP_RESP_STR_LEN              (512)

/* Sleep for 1 ms for wifi disconnect to complete */
#define THREAD_WAIT_FOR_DISCONNECT_MS  (1)

/* The interval for which the network is monitored for inactivity.
 * This application monitors for 500ms of network inactivity. Set this
 * interval as needed for the application.
 */
#define NETWORK_INACTIVE_INTERVAL_MS   (500)

/* The continuous duration for which network has to be inactive in
 * NETWORK_INACTIVE_INTERVAL_MS.
 */
#define NETWORK_INACTIVE_WINDOW_MS     (250)

/*********************************************************************
 *                           EXTERNS
 ********************************************************************/
extern cy_rslt_t app_http_server_init(WhdSTAInterface*, HTTPServer*);
extern cy_pf_ol_cfg_t *downloaded;

/*********************************************************************
 *                       GLOBAL VARIABLES
 ********************************************************************/
WhdOlmInterface     *olm;
WhdSTAInterface     *wifi;
HTTPServer          *server;
char                http_app_response[HTTP_RESP_STR_LEN] = {0};
static pf_ol_t      pf_ctxt;
Thread              T1;

/* OLM (Offload Manager) configuration that holds the reference to
 * the packet filter configuration and callback functions.
 */
ol_desc_t new_olm_list[] = {
    { "Pkt_Filter", downloaded, &pf_ol_fns, &pf_ctxt },
    { "Last", NULL, NULL, NULL }
};

/********************************************************************
 *                     FUNCTION DEFINITIONS
 *******************************************************************/
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
void app_wl_disconnect(WhdSTAInterface *wifi)
{
    nsapi_error_t   nsapi_err;

    if (wifi == NULL ) {
        MBED_APP_ERR(("%s() Bad args\n", __func__));
        return;
    }

    /* Disconnect from AP */
    nsapi_err = wifi->disconnect();
    if (nsapi_err != NSAPI_ERROR_OK ) {
        MBED_APP_ERR(("Disconnect Failed (nsapi_error):%d\n", nsapi_err));
        return;
    }

    /* Wait for the disconnect to complete */
    while( wifi->get_connection_status() != NSAPI_STATUS_DISCONNECTED) {
        ThisThread::sleep_for(THREAD_WAIT_FOR_DISCONNECT_MS);
    }
}

/*******************************************************************************
* Function Name: app_wl_print_connect_status
********************************************************************************
*
* Summary:
* This function gets the WiFi connection status and print the status on the kit's
* serial terminal.
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR.
*
*******************************************************************************/
cy_rslt_t app_wl_print_connect_status(WhdSTAInterface *wifi)
{
    cy_rslt_t ret = CY_RSLT_TYPE_ERROR;

    if (wifi == NULL)
    {
        MBED_APP_ERR(("Invalid WiFi interface passed. Cannot fetch connection status.\n"));
        return ret;
    }

    nsapi_connection_status_t status = wifi->get_connection_status();
    MBED_APP_INFO(("IP: %s\n", wifi->get_ip_address()));

    switch (status)
    {
        case NSAPI_STATUS_LOCAL_UP:
            MBED_APP_INFO(("CONNECT_STATUS: LOCAL UP.\nWiFi connection already established. "
                           "IP: %s\n", wifi->get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_GLOBAL_UP:
            MBED_APP_INFO(("CONNECT_STATUS: GLOBAL UP.\nWiFi connection already established. "
                           "IP: %s\n", wifi->get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_CONNECTING:
            MBED_APP_INFO(("CONNECT_STATUS: CONNECTING\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
        case NSAPI_STATUS_ERROR_UNSUPPORTED:
        default:
            MBED_APP_INFO(("CONNECT_STATUS: UNSUPPORTED\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
    }

    return ret;
}

/*****************************************************************************************
* Function Name: app_wl_connect
******************************************************************************************
*
* Summary:
* This function tries to connect the kit to the given AP (Access Point).
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
* const char *ssid: WiFi AP SSID.
* const char *pass: WiFi AP Password.
* nsapi_security_t secutiry: WiFi security type as defined in structure nsapi_security_t.
*
* Return:
* cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR indicating whether the kit
* connected to the given AP successfully or not.
*
*****************************************************************************************/
cy_rslt_t app_wl_connect(WhdSTAInterface *wifi, const char *ssid,
                         const char *pass, nsapi_security_t security)
{
    cy_rslt_t ret = CY_RSLT_SUCCESS;

    MBED_APP_INFO(("SSID: %s, Security: %d\n", ssid, security));

    if ((wifi == NULL) || (ssid == NULL) || (pass == NULL)) {
        MBED_APP_INFO(("Error: %s( %p, %p, %p) bad args\n", __func__, (void*)wifi, (void*)ssid, (void*)pass));
        return CY_RSLT_TYPE_ERROR;
    }

    if (wifi->get_connection_status() != NSAPI_STATUS_DISCONNECTED)
    {
        return app_wl_print_connect_status(wifi);
    }

    /* Connect network */
    MBED_APP_INFO(("\nConnecting to %s...\n", ssid));

    ret = wifi->connect(ssid, pass, security);

    if (CY_RSLT_SUCCESS == ret) {
        MBED_APP_INFO(("MAC\t: %s\n", wifi->get_mac_address()));
        MBED_APP_INFO(("Netmask\t: %s\n", wifi->get_netmask()));
        MBED_APP_INFO(("Gateway\t: %s\n", wifi->get_gateway()));
        MBED_APP_INFO(("RSSI\t: %d\n\n", wifi->get_rssi()));
        MBED_APP_INFO(("IP Addr\t: %s\n\n", wifi->get_ip_address()));
    } else {
        MBED_APP_INFO(("\nWiFi connection ERROR: %ld\n", ret));
        ret = CY_RSLT_TYPE_ERROR;
    }

    return ret;
}

/*******************************************************************************
* Function Name: restart_olm
********************************************************************************
*
* Summary:
* This function restarts the OLM (Offload Manager) taking the new configuration.
* This also free up memory consumed by the old OLM config.
*
* Parameters:
* void.
*
* Return:
* void.
*
*******************************************************************************/
void restart_olm( void )
{
    WhdOlmInterface *old_olm = olm;
    WhdOlmInterface *new_olm = new WhdOlmInterface(new_olm_list);

    new_olm_list[0].cfg = downloaded;

    if (new_olm != NULL)
    {
        wifi->set_olm(new_olm);
        if (old_olm != NULL)
        {
            free(old_olm);
            olm = new_olm;
        }
    }
    else
    {
        MBED_APP_ERR(("%s() ERROR: Failed to get new WhdOlmInterface!\r\n", __func__));
    }

}

/*******************************************************************************
* Function Name: host_sleep_action_thread
********************************************************************************
*
* Summary:
* This function is responsible for suspending the host network stack and resume
* when any TX or RX activity happens. Suspending the network stack in mbedOS
* will cause the host MCU to enter into deep-sleep and hence reduces the power
* consumption.
*
* Parameters:
* void.
*
* Return:
* void.
*
*******************************************************************************/
void host_sleep_action_thread(void)
{
    do
    {
        /* Suspend the network stack so that
         * PSoC 6 MCU can enter deep-sleep power mode.
         */
        wait_net_suspend(static_cast<WhdSTAInterface*>(wifi),
                         osWaitForever,
                         NETWORK_INACTIVE_INTERVAL_MS,
                         NETWORK_INACTIVE_WINDOW_MS);
    } while(1);
}

/*****************************************************************************************
* Function Name: main()
******************************************************************************************
*
* Summary:
* Entry function of this application. This initializes WLAN device as station interface
* and joins to AP whose details such as SSID and Password etc., need to be mentioned
* in mbed_app.json file. This also initializes and starts HTTP web server.
*
*****************************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initializes OLM with packet filter(s) configured
     * via device configurator. */
    MBED_APP_INFO(("Packet Filter Offload Demo\n\n"));
    wifi = new WhdSTAInterface();

    /* Connect to the configured WiFi AP */
    result = app_wl_connect(wifi, MBED_CONF_APP_WIFI_SSID,
                              MBED_CONF_APP_WIFI_PASSWORD,
                             MBED_CONF_APP_WIFI_SECURITY);
    
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("Failed to connect to AP. Check WiFi credentials in mbed_app.json.\n"));
        return 0;
    }

    /* Initializes and starts HTTP Web Server */
    result = app_http_server_init(static_cast<WhdSTAInterface*>(wifi),
                                  static_cast<HTTPServer*>(server));
    if (CY_RSLT_SUCCESS != result)
    {
        MBED_APP_ERR(("HTTP Server init failed\r\n"));
    }

    /* Start application thread.
     * Keep the Host MCU in low power mode by suspending the network
     * stack and resume only when there is any TX or RX activity.
     */
    T1.start(host_sleep_action_thread);

    return 0;
}

