/******************************************************************************
 * File Name: main.cpp
 *
 * Description:
 *   This application demonstrates WLAN Packet Filter offload functionality.
 *   The WLAN packet filter allows the host processor to limit what types of
 *   packet get passed up to the host from the WLAN subsystem. So the Host gets
 *   greater chance of staying in deep sleep and wake up only when the intended
 *   WLAN packet arrives. This application hosts a HTTP webserver that allows
 *   the user to modify the default packet filters and update them dynamically
 *   when the kit is running this application.
 *
 * Related Document: README.md
 *                   AN227910 Low-Power System Design with CYW43012 and PSoC 6
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

#include "mbed.h"
#include "http_webserver_config.h"

/******************************************************************************
 *                           MACROS
 *****************************************************************************/
/* This macro specifies the interval in milliseconds that the device monitors
 * the network for inactivity. If the network is inactive for duration lesser
 * than NETWORK_INACTIVE_WINDOW_MS in this interval, the MCU does not suspend
 * the network stack and informs the calling function that the MCU wait period
 * timed out while waiting for network to become inactive.
 */
#define NETWORK_INACTIVE_INTERVAL_MS   (500)

/* This macro specifies the continuous duration in milliseconds for which the
 * network has to be inactive. If the network is inactive for this duaration,
 * the MCU will suspend the network stack. Now, the MCU will not need to service
 * the network timers which allows it to stay longer in sleep/deepsleep.
 */
#define NETWORK_INACTIVE_WINDOW_MS     (250)

/******************************************************************************
 *                       GLOBAL VARIABLES
 *****************************************************************************/
/* Wi-Fi (STA) object handle.*/
WhdSTAInterface *wifi;

/* Thread handle to suspend/resume the host network stack. */
Thread T1;

/******************************************************************************
 *                     FUNCTION DEFINITIONS
 *****************************************************************************/
/******************************************************************************
 * Function Name: app_wl_disconnect
 ******************************************************************************
 * Summary:
 *   This function disconnects the Wi-Fi connection from AP.
 *
 * Parameters:
 *   wifi: A pointer to WLAN interface whose emac activity is being monitored.
 *
 * Return:
 *   void
 *
 *****************************************************************************/
void app_wl_disconnect(WhdSTAInterface *wifi)
{
    if (NULL == wifi)
    {
        ERR_INFO(("%s() Bad args\n", __func__));
        return;
    }

    /* Disconnect from AP */
    if (NSAPI_ERROR_OK != wifi->disconnect())
    {
        ERR_INFO(("Failed to disconnect from AP.\n"));
    }
}

/******************************************************************************
 * Function Name: app_wl_print_connect_status
 ******************************************************************************
 * Summary:
 *   This function gets the WiFi connection status and print the status on the
 *   kit's serial terminal.
 *
 * Parameters:
 *   wifi: A pointer to WLAN interface whose emac activity is being monitored.
 *
 * Return:
 *   cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR.
 *
 *****************************************************************************/
cy_rslt_t app_wl_print_connect_status(WhdSTAInterface *wifi)
{
    cy_rslt_t ret = CY_RSLT_TYPE_ERROR;
    nsapi_connection_status_t status = NSAPI_STATUS_DISCONNECTED;
    SocketAddress sock_addr;

    if (NULL == wifi)
    {
        ERR_INFO(("Invalid WiFi Interface. "
                  "Cannot fetch connection status.\n"));
        return ret;
    }

    /* Get IP address */
    wifi->get_ip_address(&sock_addr);

    /* Get Wi-Fi connection status */
    status = wifi->get_connection_status();

    switch (status)
    {
        case NSAPI_STATUS_LOCAL_UP:
            APP_INFO(("CONNECT_STATUS: LOCAL UP.\nWiFi connection already "
                      "established. IP: %s\n", sock_addr.get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_GLOBAL_UP:
            APP_INFO(("CONNECT_STATUS: GLOBAL UP.\nWiFi connection already "
                      "established. IP: %s\n", sock_addr.get_ip_address()));
            ret = CY_RSLT_SUCCESS;
            break;
        case NSAPI_STATUS_CONNECTING:
            APP_INFO(("CONNECT_STATUS: CONNECTING\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
        case NSAPI_STATUS_ERROR_UNSUPPORTED:
        default:
            APP_INFO(("CONNECT_STATUS: UNSUPPORTED\n"));
            ret = CY_RSLT_TYPE_ERROR;
            break;
    }

    return ret;
}

/******************************************************************************
 * Function Name: app_wl_connect
 ******************************************************************************
 * Summary:
 *   This function tries to connect the kit to the given AP (Access Point).
 *
 * Parameters:
 *   wifi: A pointer to WLAN interface whose emac activity is being monitored.
 *   ssid: WiFi AP SSID.
 *   pwd: WiFi AP Password.
 *   secutiry: WiFi security type as defined in structure nsapi_security_t.
 *
 * Return:
 *   cy_rslt_t: Returns CY_RSLT_SUCCESS or CY_RSLT_TYPE_ERROR indicating
 *     whether the kit connected to the given AP successfully or not.
 *
 *****************************************************************************/
cy_rslt_t app_wl_connect(WhdSTAInterface *wifi, const char *ssid,
                         const char *pwd, nsapi_security_t security)
{
    cy_rslt_t ret = CY_RSLT_SUCCESS;
    SocketAddress sock_addr;

    APP_INFO(("SSID: %s, Security: %d\n", ssid, security));

    if ((NULL == wifi) || (NULL == ssid) || (NULL == pwd))
    {
        ERR_INFO(("Incorrect Wi-Fi credentials.\n"));
        return CY_RSLT_TYPE_ERROR;
    }

    /* Check if the Wi-Fi is disconnected from AP. */
    if (NSAPI_STATUS_DISCONNECTED != wifi->get_connection_status())
    {
        return app_wl_print_connect_status(wifi);
    }

    APP_INFO(("Connecting to Wi-Fi AP: %s\n", ssid));
    ret = wifi->connect(ssid, pwd, security);

    if (CY_RSLT_SUCCESS == ret)
    {
        APP_INFO(("MAC\t : %s\n", wifi->get_mac_address()));
        wifi->get_netmask(&sock_addr);
        APP_INFO(("Netmask\t : %s\n", sock_addr.get_ip_address()));
        wifi->get_gateway(&sock_addr);
        APP_INFO(("Gateway\t : %s\n", sock_addr.get_ip_address()));
        APP_INFO(("RSSI\t : %d\n\n", wifi->get_rssi()));
        wifi->get_ip_address(&sock_addr);
        APP_INFO(("IP Addr\t : %s\n\n", sock_addr.get_ip_address()));
    }
    else
    {
        APP_INFO(("\nFailed to connect to Wi-Fi AP.\n"));
        ret = CY_RSLT_TYPE_ERROR;
    }

    return ret;
}

/******************************************************************************
* Function Name: host_sleep_action_thread
*******************************************************************************
* Summary:
*   This function is responsible for suspending the host network stack. The
*   network stack resumes when any TX or RX activity detected in the Wi-Fi
*   driver interface. Suspending the network stack will cause the host MCU to
*   enter the deep sleep power mode.
*
* Parameters:
*   void
*
* Return:
*   void
*
******************************************************************************/
void host_sleep_action_thread(void)
{
    do
    {
        /* Configures an emac activity callback to the Wi-Fi interface
         * and suspends the network stack if the network is inactive for
         * a duration of INACTIVE_WINDOW_MS inside an interval of
         * INACTIVE_INTERVAL_MS. The callback is used to signal the
         * presence/absence of network activity to resume/suspend the
         * network stack.
         */
        wait_net_suspend(static_cast<WhdSTAInterface*>(wifi),
                         osWaitForever,
                         NETWORK_INACTIVE_INTERVAL_MS,
                         NETWORK_INACTIVE_WINDOW_MS);
    } while(1);
}

/******************************************************************************
* Function Name: main()
*******************************************************************************
*
* Summary:
*   Entry function of this application. This initializes WLAN device as station
*   interface and joins to AP whose details such as Wi-Fi SSID, Password, and
*   Security type need to be mentioned in mbed_app.json file. This also
*   initializes and starts HTTP web server.
*
******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen */
    APP_INFO(("\x1b[2J\x1b[;H"));
    APP_INFO(("=======================================\n"));
    APP_INFO(("PSoC 6 MCU: Packet Filter Offload Demo\n"));
    APP_INFO(("=======================================\n\n"));

    /* Initializes OLM with packet filter(s) configured
     * via device configurator. */
    wifi = new WhdSTAInterface();

    /* Connect to the configured WiFi AP */
    result = app_wl_connect(wifi, MBED_CONF_APP_WIFI_SSID,
                              MBED_CONF_APP_WIFI_PASSWORD,
                             MBED_CONF_APP_WIFI_SECURITY);
    PRINT_AND_ASSERT(result, "Failed to connect to AP. "
                     "Check Wi-Fi credentials in mbed_app.json file.\n");

    /* Initializes and starts HTTP Web Server */
    app_http_server_init(static_cast<WhdSTAInterface*>(wifi));

    /* Start application thread.
     * Keep the Host MCU in low power mode by suspending the network
     * stack and resume only when there is any Tx/Rx activity detected.
     */
    T1.start(host_sleep_action_thread);

    return 0;
}


/* [] END OF FILE */

