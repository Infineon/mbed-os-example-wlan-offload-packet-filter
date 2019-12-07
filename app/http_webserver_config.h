/********************************************************************************
 * File Name: http_webserver_config.h
 *
 * Version: 1.0
 *
 * Description:
 *   This is the header file and contains macro definition and function declarations
 *   for the functions defined in http_webserver_config.cpp file.
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

#ifndef HTTP_WEBSERVER_CONFIG_H
#define HTTP_WEBSERVER_CONFIG_H

#include "mbed.h"
#include "HTTP_server.h"
#include "WhdSTAInterface.h"

/*********************************************************************
 *                           MACROS
 ********************************************************************/
#define HTTP_RESP_STR_BUFFER_LEN   (2048)
#define HTTP_BUILD_STR_LEN         (400)
#define HTTP_QUERY_STR_VALUE_LEN   (50)
#define HTTP_PORT                  (80u)
#define MAX_SOCKETS                (2u)
#define MAX_RETRIES                (3u)
#define APP_INFO( x )              printf x
#define ERR_INFO( x )              printf x

/*********************************************************************
 *                      FUNCTION DECLARATIONS
 ********************************************************************/
/*******************************************************************************
* Function Name: http_startup_webpage
********************************************************************************
*
* Summary:
* This function builds HTTP string with web buttons and text boxes and send the
* string as response text to the web server. It will fetch the active packet
* filters and pending packet filter lists and appends the configuration to the
* HTTP response string builder. This will be sent as HTTP response string to the
* web server and the data gets populated in the web page.
*
* Parameters:
* const char* url_path: Pointer to HTTP url path.
* const char* url_query_string: Pointer to HTTP url query string.
* cy_http_response_stream_t* stream: Pointer to HTTP server stream through which
* HTTP data sent/received.
* void* arg: Argument as set in callback registration.
* cy_http_message_body_t* http_data: Pointer to HTTP data.
*
* Return:
* int32_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
int32_t   http_startup_webpage(const char* url_path, const char* url_query_string,
                                     cy_http_response_stream_t* stream, void* arg,
                                               cy_http_message_body_t* http_data);

/*******************************************************************************
* Function Name: http_configure_filter
********************************************************************************
*
* Summary:
* This function will redirect to another web page for configuring packet filters.
* 
* Parameters:
* const char* url_path: Pointer to HTTP url path.
* const char* url_query_string: Pointer to HTTP url query string.
* cy_http_response_stream_t* stream: Pointer to HTTP server stream through which
* HTTP data sent/received.
* void* arg: Argument as set in callback registration.
* cy_http_message_body_t* http_data: Pointer to HTTP data.
*
* Return:
* int32_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
int32_t   http_configure_filter(const char* url_path, const char* url_query_string,
                                      cy_http_response_stream_t* stream, void* arg,
                                                cy_http_message_body_t* http_data);

/*******************************************************************************
* Function Name: http_submit_filter
********************************************************************************
*
* Summary:
* This function will add the packet filter to the pending packet filter list and
* will redirect back to the home web page. Note that the configuration is still
* in pending list and not applied to the WLAN device. To apply the pending
* configuration list, you need to click Apply Filters web button.
*
* Parameters:
* const char* url_path: Pointer to HTTP url path.
* const char* url_query_string: Pointer to HTTP url query string.
* cy_http_response_stream_t* stream: Pointer to HTTP server stream through which
* HTTP data sent/received.
* void* arg: Argument as set in callback registration.
* cy_http_message_body_t* http_data: Pointer to HTTP data.
*
* Return:
* cy_rslt_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
cy_rslt_t http_submit_filter(cy_http_message_body_t* http_data);

/*******************************************************************************
* Function Name: app_http_server_init
********************************************************************************
*
* Summary:
* This function is responsible for initializing the HTTP web server.
* It initializes with all the callbacks required, creates web link resources
* and then starts the web server.
*
* Parameters:
* WhdSTAInterface *wifi: A pointer to WLAN interface whose emac activity is being
* monitored.
* HTTPServer *server: Pointer to HTTP server instance.
* 
* Return:
* cy_rslt_t: Returns error code as defined in cy_rslt_t.
*
*******************************************************************************/
cy_rslt_t app_http_server_init(WhdSTAInterface *wifi, HTTPServer *server);

/*********************************************************************
 *                       GLOBAL VARIABLES
 ********************************************************************/
cy_resource_dynamic_data_t test_data                 = {http_startup_webpage/*startup_page*/, NULL};
cy_resource_dynamic_data_t http_configure_filter_url = {http_configure_filter, NULL};

#endif //HTTP_WEBSERVER_CONFIG_H

