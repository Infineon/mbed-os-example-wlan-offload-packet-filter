/******************************************************************************
 * File Name: http_webserver_config.h
 *
 * Description:
 *   This header file contains macros and function declarations for HTTP server
 *   functionality.
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

#ifndef HTTP_WEBSERVER_CONFIG_H
#define HTTP_WEBSERVER_CONFIG_H

#include "mbed.h"
#include "HTTP_server.hpp"
#include "WhdSTAInterface.h"
#include "network_activity_handler.h"

/******************************************************************************
 *                               MACROS
 *****************************************************************************/
#define HTTP_RESP_STR_BUFFER_LEN   (2048)
#define HTTP_BUILD_STR_LEN         (400)
#define HTTP_QUERY_STR_VALUE_LEN   (50)
#define HTTP_PORT                  (80u)
#define MAX_SOCKETS                (2u)

#define APP_INFO(x)                do { printf("Info: "); printf x; } while(0);
#define ERR_INFO(x)                do { printf("Error: "); printf x; } while(0);

#define PRINT_AND_ASSERT(result, msg, args...)   \
                                   do                                 \
                                   {                                  \
                                       if (CY_RSLT_SUCCESS != result) \
                                       {                              \
                                           ERR_INFO((msg, ## args));  \
                                           MBED_ASSERT(0);            \
                                       }                              \
                                   } while(0);

/******************************************************************************
 *                        FUNCTION DECLARATIONS
 *****************************************************************************/
int32_t http_startup_webpage(const char* url_path,
                             const char* url_query_string,
                             cy_http_response_stream_t* stream,
                             void* arg,
                             cy_http_message_body_t* http_data);
int32_t http_configure_filter(const char* url_path,
                              const char* url_query_string,
                              cy_http_response_stream_t* stream,
                              void* arg,
                              cy_http_message_body_t* http_data);
cy_rslt_t app_wl_connect(WhdSTAInterface *wifi,
                         const char *ssid,
                         const char *pwd,
                         nsapi_security_t security);
cy_rslt_t http_submit_filter(cy_http_message_body_t* http_data);
void app_wl_disconnect(WhdSTAInterface *wifi);
void app_http_server_init(WhdSTAInterface *wifi);
int8_t *get_entry_id(void);

#endif /* #ifndef HTTP_WEBSERVER_CONFIG_H */


/* [] END OF FILE */

