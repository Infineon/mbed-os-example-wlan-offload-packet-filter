/******************************************************************************
 * File Name: http_webserver_config.cpp
 *
 * Description:
 *   This file contains code for HTTP webserver operations such as displaying
 *   the HTTP startup web page, configuring packet filters, removing filters,
 *   adding a new packet filter and restore defaults etc.
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

#include "http_webserver_config.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "WhdOlmInterface.h"
#include "cy_lpa_wifi_ol.h"
#include "pf_olm_config.h"

/******************************************************************************
 *                              EXTERNS
 *****************************************************************************/
extern cy_pf_ol_cfg_t *downloaded;

/******************************************************************************
 *                         GLOBAL VARIABLES
 *****************************************************************************/
static int8_t entry_id = 0;
HTTPServer *server;

static char http_text_heading[] =
"<html><h1>Packet Filter Offload</h1>"
  "<body>"
    "<font size=\"4\" color=\"red\" style=\"font-style:italic;\">"
    "Maximum %d filters can be configured. The HTTP server will discard the additional filters.</font>"
    "<br><br>"
  "</body>"
"</html>";

static char http_text_start[] = 
"<html>"
  "<body>"
    "<style>"
      ".container {"
        "width: 80%;"
        "height: 200px;"
      "}"
      ".one {"
        "width: 50%;"
        "height: 200px;"
        "float: left;"
      "}"
      ".two {"
        "margin-left: 50%;"
        "height: 200px;"
      "}"
      ".three {"
        "background-color: lightpink;"
        "padding: 8px;"
        "color: black;"
        "width: 150px;"
      "}"
    "</style>"
    "<script>"
      "function changeVal(obj, value, action) {"
        "obj.value = value;"
        "obj.formAction = action;"
      "}"
      "function validateBeforeApply() {"
        "if (confirm('Warning! Applying the new packet filter(s) "
                     "list will overwrite the current active filter(s). "
                     "The webpage will temporarily go down as the kit "
                     "will disconnect and rejoin the AP. Stop the current "
                     "page from loading and browse to root URL. Proceed to apply ?'))"
        "{"
            "if (document.getElementById(\"pending_list\").value == '')"
            "{"
                "if (confirm('List is empty. Applying empty list means no packet filters "
                     "are configured. Proceed to apply ?'))"
                "{"
                    "return true;"
                "} else {"
                    "return false;"
                "}"
            "}"
            "return true;"
        "}"
      "}"
    "</script>"
    "<form name=\"apply_filter\">"
      "<section class=\"container\">"
        "<div class=\"one\">";
static char http_text_end[] =
      "</section>"
    "</form>"
  "</body>"
"</html>";

static char configure_pkt_filter_webpage[] =
"<html>"
  "<body onload=\"show_hide('PF');\">"
    "<script>"
      "function show_hide(val) {"
        "if(val == \"PF\") {"
          "document.getElementById(\"id3\").style.display = '';"
          "document.getElementById(\"id4\").style.display = '';"
          "document.getElementById(\"id5\").style.display = '';"
          "document.getElementById(\"id6\").style.display = 'none';"
          "document.getElementById(\"id7\").style.display = 'none';"
        "} else if(val == \"ET\") {"
          "document.getElementById(\"id6\").style.display = '';"
          "document.getElementById(\"id3\").style.display = 'none';"
          "document.getElementById(\"id4\").style.display = 'none';"
          "document.getElementById(\"id5\").style.display = 'none';"
          "document.getElementById(\"id7\").style.display = 'none';"
        "} else {"
          "document.getElementById(\"id7\").style.display = '';"
          "document.getElementById(\"id3\").style.display = 'none';"
          "document.getElementById(\"id4\").style.display = 'none';"
          "document.getElementById(\"id5\").style.display = 'none';"
          "document.getElementById(\"id6\").style.display = 'none';"
        "}"
      "}"

      "function keep_partial_text(val) {"
        "if (val.selectionStart < 2) {"
          "event.preventDefault();"
      "}"
        "if (val.selectionStart == 2 && event.keyCode == 8) {"
          "event.preventDefault();"
        "}"
      "}"

      "function validate_filter() {"
        "filter_type = document.getElementById(\"filter_type\").value;"
        "if (filter_type == 'PF') { "
          "port_number = document.getElementById(\"port_number\").value;"
          "if (port_number < 0 || port_number > 65535) {"
            "alert('Invalid port number specified. Valid range: 0-65535.');"
            "return false;"
          "}"
        "}"
        "if (filter_type == 'ET') { "
          "ether_type = document.getElementById(\"ether_type\").value;"
          "if ((parseInt(ether_type, 16) < 2048) || (parseInt(ether_type, 16) > 65535)) {"
            "alert('Invalid Eth type specified. Valid range: 0x0800-0xFFFF.');"
            "return false;"
          "}"
        "}"
        "if (filter_type == 'IT') { "
          "ip_proto = document.getElementById(\"ip_proto\").value;"
          "if ((parseInt(ip_proto, 16) < 1) || (parseInt(ip_proto, 16) > 255)) {"
            "alert('Invalid IP type specified. Valid range: 0x01-0xFF.');"
            "return false;"
          "}"
        "}"
        "return true;"
      "}"
    "</script>"
    "<form method=\"POST\" action=\"/?add=add\" onsubmit=\"return validate_filter()\">"
    "<table>"
      "<tr id=\"id1\">"
        "<td id=\"cell1\">Filter Type:</td>"
        "<td id=\"cell2\">"
          "<select id=\"filter_type\" name=\"filter_type\" onchange=\"show_hide(this.value);\">"
            "<option value=\"PF\">Port Filter</option>"
            "<option value=\"ET\">Ether Type</option>"
            "<option value=\"IT\">IP Type</option>"
          "</select>"
        "</td>"
      "</tr>"
      "<tr id=\"id2\">"
        "<td id=\"cell3\">Action:</td>"
        "<td id=\"cell4\">"
          "<select id=\"action\" name=\"action\">"
            "<option value=\"K\">Keep</option>"
            "<option value=\"D\">Discard</option>"
          "</select>"
        "</td>"
      "</tr>"
      "<tr id=\"id3\">"
        "<td id=\"cell5\">Protocol:</td>"
        "<td id=\"cell6\">"
          "<select id=\"protocol\" name=\"protocol\">"
            "<option value=\"T\">TCP</option>"
            "<option value=\"U\">UDP</option>"
          "</select>"
        "</td>"
      "</tr>"
      "<tr id=\"id4\">"
        "<td id=\"cell7\">Direction:</td>"
        "<td id=\"cell8\">"
          "<select id=\"direction\" name=\"direction\">"
            "<option value=\"SP\">Source Port</option>"
            "<option value=\"DP\" selected>Destination Port</option>"
          "</select>"
        "</td>"
      "</tr>"
      "<tr id=\"id5\">"
        "<td id=\"cell9\">Port Number:</td>"
        "<td id=\"cell10\">"
          "<input id=\"port_number\" name=\"port_number\" type=\"text\" onkeypress=\"return (event.keyCode >= 48 && event.keyCode <= 57)\"/> (0 - 65535)"
        "</td>"
      "</tr>"
      "<tr id=\"id6\">"
        "<td id=\"cell11\">Ether Type:</td>"
        "<td id=\"cell12\">"
          "<input id=\"ether_type\" name=\"ether_type\" type=\"text\" value=\"0x\""
            "onkeypress=\"return (event.keyCode >= 48 && event.keyCode <= 57) || (event.keyCode >= 65 && event.keyCode <= 70) ||"
            "(event.keyCode >= 97 && event.keyCode <= 102)\" onkeydown=\"keep_partial_text(this);\"/> (0x0800 - 0xFFFF)"
        "</td>"
      "</tr>"
      "<tr id=\"id7\">"
        "<td id=\"cell13\">IP Protocol:</td>"
        "<td id=\"cell14\">"
          "<input id=\"ip_proto\" name=\"ip_proto\" type=\"text\" value=\"0x\""
            "onkeypress=\"return (event.keyCode >= 48 && event.keyCode <= 57) || (event.keyCode >= 65 && event.keyCode <= 70) ||"
            "(event.keyCode >= 97 && event.keyCode <= 102)\" onkeydown=\"keep_partial_text(this);\"/> (0x01 - 0xFF)"
        "</td>"
      "</tr>"
    "</table>"
      "<input type=\"submit\" name=\"add\" value=\"Submit\">"
    "</form>"
    "<label type=\"text\" style=\"color: maroon;\">Note:<br>"
      "<ul type=\"square\">"
        "<li>Adding combination of \"Keep\" and \"Discard\" filters are not allowed."
        " The filter action should follow only \"Keep\" filters or only \"Discard\" filters.</li>"
        "<li>Duplicate filters will be rejected and not added to the pending filter list.</li>"
        "<li> Only one Discard filter can be added. If any discard filter already exists in the"
        " pending list, then no further filter can be added and the operation will be rejected.</li>"
        "<li>Packet types:</li>"
          "<ul type=\"disc\">"
            "<li><a href=\"https://www.iana.org/protocols\">IANA Assigned numbers (Trusted source)</a></li>"
            "<li><a href=\"https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers\">TCP and UDP Port numbers</a></li>"
            "<li><a href=\"https://en.wikipedia.org/wiki/EtherType\">Ether Type numbers</a></li>"
            "<li><a href=\"https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers\">IP protocol numbers</a></li>"
          "</ul>"
      "</ul>"
    "</label>"
    "</body>"
    "</html>";

/* HTML resources to register with the HTTP server. */
cy_resource_dynamic_data_t test_data = {http_startup_webpage, NULL};
cy_resource_dynamic_data_t http_configure_filter_url = {http_configure_filter, NULL};

/******************************************************************************
 *                     FUNCTION DEFINITIONS
 *****************************************************************************/
/******************************************************************************
 * Function Name: add_remove_restore_filters
 ******************************************************************************
 * Summary:
 *   This function is called when the user selects any of these buttons from
 *   the webpage: Add Filter, Remove Last Filter, Import minimal keep filters,
 *   and Restore Defaults.
 *   Add Filter: Redirects to another page to configure and add a new packet
 *   filter to the pending list.
 *   Remove Last Filter: Removes the last added filter from the pending list.
 *   Import minimal keep filters: Pulls the list of minimum keep filters
 *   (default filters) as selected in the device configurator tool.
 *   Restore Defaults: Applies the default packet filters as selected in the
 *   device configurator tool.
 *
 * Parameters:
 *   query_string: Pointer to HTTP url query string.
 *   http_data: Pointer to HTTP data.
 *
 * Return:
 *   cy_rslt_t: Returns error code as defined in cy_rslt_t indicating the
 *              operation success or not.
 *
 *****************************************************************************/
cy_rslt_t add_remove_restore_filters(const char *query_string,
                                     cy_http_message_body_t *http_data)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (NULL != query_string)
    {
        if (!strncmp(query_string, "add", strlen(query_string)-1))
        {
            /* Add a new filter to the pending list */
            result = http_submit_filter(http_data);
        }
        else if (!strncmp(query_string, "remove", strlen(query_string)-1))
        {
            /* Remove last added filter from pending list */
            result = remove_last_added_filter();
        }
        else if (!strncmp(query_string, "restore_defaults", strlen(query_string)-1))
        {
            /* Restore default packet filter configs */
            result = pf_commit_list(true);
        }
        else if (!strncmp(query_string, "minimum_filter", strlen(query_string)-1))
        {
            /* Add minimum required packet filters to the pending list */
            add_minimum_filters();
        }
        else if (!strncmp(query_string, "apply_filter", strlen(query_string)-1))
        {
            result = pf_commit_list(false);
        }
    }

    return result;
}

/******************************************************************************
* Function Name: http_startup_webpage
*******************************************************************************
* Summary:
*   This function builds HTTP string with web buttons and text boxes and send
*   the string as response text to the web server. It will fetch the active
*   packet filters and pending packet filter lists and appends the configuration
*   to the HTTP response string builder. This will be sent as HTTP response
*   string to the web server and the data gets populated in the web page.
*
* Parameters:
*   url_path: Pointer to HTTP url path.
*   url_query_string: Pointer to HTTP url query string.
*   stream: Pointer to HTTP server stream through which HTTP data sent/received.
*   arg: Argument as set in callback registration.
*   http_data: Pointer to HTTP data.
*
* Return:
*   int32_t: Returns error code as defined in cy_rslt_t.
*
******************************************************************************/
int32_t http_startup_webpage(const char *url_path,
                             const char *url_query_string,
                             cy_http_response_stream_t *stream,
                             void *arg,
                             cy_http_message_body_t *http_data)
{
    char http_resp_str_builder[HTTP_RESP_STR_BUFFER_LEN] = {0};
    char parse_query_string[HTTP_QUERY_STR_VALUE_LEN] = {0};
    char build_str[HTTP_BUILD_STR_LEN] = {0};
    cy_pf_ol_cfg_t *cfg = downloaded;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Parse URL query string. The possible user actions are
     * Add/Remove/Import/Restore/Apply filters.
     */
    memcpy(parse_query_string, url_query_string, sizeof(parse_query_string));
    strtok(parse_query_string, "=");
    memcpy(parse_query_string, strtok(NULL, "="), sizeof(parse_query_string));

    result = add_remove_restore_filters(parse_query_string, http_data);
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to perform the user request.\n"));
    }

    /* Return here as the kit will reassociate with AP. */
    if (!strncmp(parse_query_string, "restore_defaults", strlen(parse_query_string)-1) ||
        !strncmp(parse_query_string, "apply_filter", strlen(parse_query_string)-1))
    {
        return result;
    }

    /* Initialize the home web page. The home page contains two sections:
     * Active Packet Filters and Pending Packet Filters.
     */
    memset(build_str, 0, sizeof(build_str));
    sprintf(build_str, (const char *)&http_text_heading[0], get_max_filter());
    result = server->http_response_stream_write(stream, build_str, strlen(build_str));
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to write HTTP response\r\n"));
    }

    /* Populate active packet filter list. */
    if (NULL != cfg)
    {
        strcat(http_resp_str_builder, http_text_start);
        strcat(http_resp_str_builder,
               "<b>Active Packet Filters:</b><br>"
               "<textarea readonly rows=\"4\" cols=\"50\" "
               "style=\"background:lightblue; font-size:large; "
               "height:431px; width:420px\">");
        while ((NULL != cfg) && (CY_PF_OL_FEAT_LAST != cfg->feature))
        {
            memset(build_str, 0, sizeof(build_str));
            if (CY_PF_OL_FEAT_PORTNUM == cfg->feature)
            {
                sprintf(build_str, "\nID %d[Port Filter]:\n"
                                   "\tPort = %d,\n"
                                   "\tAction = %s,\n"
                                   "\tProtocol = %s,\n"
                                   "\tDirection = %s",
                                   (cfg->id),
                                   (cfg->u.pf.portnum.portnum),
                                   ((cfg->bits & CY_PF_ACTION_DISCARD)?"Discard":"Keep"),
                                   ((cfg->u.pf.proto == 1)?"UDP":"TCP"),
                                   ((cfg->u.pf.portnum.direction == 1)?"Destination Port":"Source Port"));
                strcat(http_resp_str_builder, build_str);
            }
            else if (CY_PF_OL_FEAT_ETHTYPE == cfg->feature)
            {
                sprintf(build_str, "\nID %d[Eth Filter]:\n"
                                   "\tPacket Type = 0x%x,\n"
                                   "\tAction = %s",
                                   (cfg->id),
                                   (cfg->u.eth.eth_type),
                                   ((cfg->bits & CY_PF_ACTION_DISCARD)?"Discard":"Keep"));
                strcat(http_resp_str_builder, build_str);
            }
            else if (CY_PF_OL_FEAT_IPTYPE == cfg->feature)
            {
                sprintf(build_str, "\nID %d[IP Filter]:\n"
                                   "\tPacket Type = 0x%x,\n"
                                   "\tAction = %s",
                                   (cfg->id),
                                   (cfg->u.ip.ip_type),
                                   ((cfg->bits & CY_PF_ACTION_DISCARD)?"Discard":"Keep"));
                strcat(http_resp_str_builder, build_str);
            }

            /* Get the next configuration. */
            cfg++;
            strcat(http_resp_str_builder, "\n");
            result = server->http_response_stream_write(stream,
                                                        http_resp_str_builder,
                                                        strlen(http_resp_str_builder));
            if (CY_RSLT_SUCCESS != result)
            {
                ERR_INFO(("Failed to write HTTP response\r\n"));
            }
            memset(http_resp_str_builder, '\0', sizeof(http_resp_str_builder));
        }
    }

    strcat(http_resp_str_builder,
           "</textarea><br><button class=\"three\" type=\"submit\""
           "name=\"restore\" onclick=\"confirm('This will cause the "
           "target kit to reassociate to AP. Stop the current page from "
           "loading and refresh the root URL. Proceed to restore defaults ?"
           "')?changeVal(this, 'restore_defaults',"
           "'/?restore'):changeVal(this, '', '');\">Restore defaults"
           "</button></div>");

    /* Send the populated active packet filters to the client. */
    result = server->http_response_stream_write(stream, http_resp_str_builder,
                                               sizeof(http_resp_str_builder));
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to write HTTP response\r\n"));
    }

    /* Initialize the pending packet filter list. */
    memset(http_resp_str_builder, '\0', sizeof(http_resp_str_builder));
    memset(build_str, '\0', sizeof(build_str));
    strcat(http_resp_str_builder,
           "<div class=\"two\"><b>Pending Packet Filters:</b><table><tr><td>"
           "<textarea id=\"pending_list\" readonly rows=\"4\" cols=\"50\" "
           "style=\"background:lightblue;font-size:large; height:431px; width:420px\">");

    /* Populate the Pending Packet Filter list. */
    cfg = get_pending_filter_list();
    cy_pf_ol_cfg_t *end_cfg = (cfg + MAX_FILTERS);
    for (; (NULL != cfg) && (cfg->feature != CY_PF_OL_FEAT_LAST) && (cfg < end_cfg); cfg++)
    {
        print_filter(cfg, &http_resp_str_builder[0], &build_str[0], sizeof(build_str));
    }
    strcat(http_resp_str_builder,
           "</textarea></td><td>"
           "<button class=\"three\" type=\"submit\" formaction=\"configure_filter\">Add Filter</button><br><br>"
           "<button class=\"three\" type=\"submit\" name=\"remove\" value=\"remove\" formaction=\"/?remove\">Remove Last Filter</button><br><br>"
           "<button class=\"three\" type=\"submit\" name=\"minimum_filter\" onclick=\"confirm('Add minimum keep packet filters "
           "to the pending list ?')?changeVal(this, 'minimum_filter', '/?minimum_filter'):changeVal(this, '', '')\">Import minimal keep filters</button>"
           "<br><br>"
           "<button class=\"three\" type=\"submit\" name=\"apply_filter\""
           "onclick=\"if(validateBeforeApply()){ changeVal(this, 'apply_filter', '/?apply_filter'); }else{ changeVal(this, '', ''); }\">Apply Filters</button></td></tr></table></div>");

    strcat(http_resp_str_builder, http_text_end);
    result = server->http_response_stream_write(stream,
                                                http_resp_str_builder,
                                                strlen(http_resp_str_builder));
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to write HTTP response\r\n"));
    }

    return result;
}

/******************************************************************************
* Function Name: http_configure_filter
*******************************************************************************
* Summary:
*   This function will redirect to another web page for configuring
*   packet filters.
*
* Parameters:
*   url_path: Pointer to HTTP url path.
*   url_query_string: Pointer to HTTP url query string.
*   stream: Pointer to HTTP server stream through which HTTP data sent/received.
*   arg: Argument as set in callback registration.
*   http_data: Pointer to HTTP data.
*
* Return:
*   int32_t: Returns error code as defined in cy_rslt_t.
*
******************************************************************************/
int32_t http_configure_filter(const char *url_path,
                              const char *url_query_string,
                              cy_http_response_stream_t *stream,
                              void *arg,
                              cy_http_message_body_t *http_data)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    result = server->http_response_stream_write(stream,
                                                configure_pkt_filter_webpage,
                                                sizeof(configure_pkt_filter_webpage));
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to write HTTP response\r\n"));
    }

    return result;
}

/******************************************************************************
* Function Name: parse_webpage_config
*******************************************************************************
* Summary:
*   The function helps to parse the HTTP data string.
*
* Parameters:
*   http_data: Pointer to the HTTP data string.
*   data_len: HTTP data string length.
*   config_str[]: Pointer to filter data. The filter data comes from HTTP
*     server as a string and this variable is used to hold pointer to it.
*
* Return:
*   void.
*
******************************************************************************/
static void parse_webpage_config(char *http_data,
                                 uint16_t data_len,
                                 char *config_str[])
{
    uint8_t index = 0;
    char *token = NULL;

    token = strtok(http_data, "&");

    while ((NULL != token) && (MAX_HTTP_CONFIG_NUMBER > index))
    {
        size_t len = strlen(token) + 1;
        void *copy = malloc(len);
        if (NULL == copy)
        {
            return;
        }
        config_str[index] = (char *)memcpy(copy, token, len);
        token = strtok(NULL, "&");
        index++;
    }

    index = 0;
    while ((NULL != config_str[index]) && (MAX_HTTP_CONFIG_NUMBER > index))
    {
        token = strtok(config_str[index], "=");
        config_str[index] = strtok(NULL, "=");
        index++;
    }
}

/******************************************************************************
* Function Name: get_entry_id
*******************************************************************************
* Summary:
*   This function returns the reference of entry_id static variable, giving
*   access to other modules in the application to change the entry id
*   dynamically.
*
* Parameters:
*   None.
*
* Return:
*   int8_t: Returns pointer to entry_id.
*
******************************************************************************/
int8_t *get_entry_id(void)
{
    return &entry_id;
}

/******************************************************************************
* Function Name: http_submit_filter
*******************************************************************************
* Summary:
*   This function will add the packet filter to the pending packet filter list
*   and will redirect back to the home web page. Note that the configuration is
*   still in pending list and not applied to the WLAN device. To apply the
*   pending configuration list, you need to click Apply Filters web button.
*
* Parameters:
*   url_path: Pointer to HTTP url path.
*   url_query_string: Pointer to HTTP url query string.
*   stream: Pointer to HTTP server stream through which HTTP data sent/received.
*   arg: Argument as set in callback registration.
*   http_data: Pointer to HTTP data.
*
* Return:
*   cy_rslt_t: Returns error code as defined in cy_rslt_t.
*
******************************************************************************/
cy_rslt_t http_submit_filter(cy_http_message_body_t *http_data)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    char *config_str[7] = {NULL};

    if (NULL == http_data || NULL == http_data->data)
    {
        return result;
    }

    /* Parse HTTP data string. */
    parse_webpage_config((char *)&http_data->data[0],
                          http_data->data_length,
                          config_str);

    /* Add packet filter to list */
    result = pf_add_to_list(&config_str[0], entry_id);
    
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("PF add to list failed\n"));
        entry_id--;
    }

    entry_id++;

    return result;
}

/*******************************************************************************
* Function Name: app_http_server_init
********************************************************************************
* Summary:
*   This function is responsible for initializing the HTTP web server. It
*   initializes with all the callbacks required, creates web link resources
*   and then starts the web server.
*
* Parameters:
*   wifi: A pointer to WLAN interface whose emac activity is
*   being monitored.
*
* Return:
*   void.
*
*******************************************************************************/
void app_http_server_init(WhdSTAInterface *wifi)
{
    cy_network_interface_t nw_interface;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    SocketAddress sock_addr;

    nw_interface.object = (void *)wifi;
    nw_interface.type   = CY_NW_INF_TYPE_WIFI;

    /* Initialize HTTP server object. */
    server = new HTTPServer(&nw_interface, HTTP_PORT, MAX_SOCKETS);

    /* Register HTTP page resources. */
    result = server->register_resource((uint8_t*)"/",
                                       (uint8_t*)"text/html",
                                       CY_DYNAMIC_URL_CONTENT,
                                       &test_data);
    PRINT_AND_ASSERT(result, "Registering HTTP page resource '/' failed.\n");

    result = server->register_resource((uint8_t*)"/configure_filter",
                                       (uint8_t*)"text/html",
                                       CY_DYNAMIC_URL_CONTENT,
                                       &http_configure_filter_url);
    PRINT_AND_ASSERT(result, "Registering HTTP page resource '/configure_filter' failed.\n");

    /* Start HTTP server */
    result = server->start();
    PRINT_AND_ASSERT(result, "Failed to start HTTP server.\n");

    /* Get ip address */
    wifi->get_ip_address(&sock_addr);
    APP_INFO(("HTTP server started successfully. "
              "Go to the webpage http://%s\r\n", sock_addr.get_ip_address()));
}


/* [] END OF FILE */

