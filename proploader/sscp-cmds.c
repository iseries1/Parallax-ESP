#include "esp8266.h"
#include "sscp.h"
#include "uart.h"
#include "config.h"
#include "cgiprop.h"
#include "cgiwifi.h"

// (nothing)
void ICACHE_FLASH_ATTR cmds_do_nothing(int argc, char *argv[])
{
    sscp_sendResponse("S,0");
}

static int getIPAddress(void *data, char *value)
{
    struct ip_info info;
    
    if (!wifi_get_ip_info(STATION_IF, &info))
        return -1;
        
    os_sprintf(value, "%d.%d.%d.%d", 
        (info.ip.addr >> 0) & 0xff,
        (info.ip.addr >> 8) & 0xff, 
        (info.ip.addr >>16) & 0xff,
        (info.ip.addr >>24) & 0xff);
        
    return 0;
}

static int setIPAddress(void *data, char *value)
{
    return -1;
}

static int setBaudrate(void *data, char *value)
{
    uart_drain_tx_buffer(UART0);
    uart0_baud(atoi(value));
    return 0;
}

enum {
    PIN_GPIO0 = 0,      // PGM
    PIN_GPIO2 = 2,      // DBG
    PIN_GPIO4 = 4,      // SEL
    PIN_GPIO5 = 5,      // ASC
    PIN_GPIO12 = 12,    // DTR
    PIN_GPIO13 = 13,    // CTS
    PIN_GPIO14 = 14,    // DIO9
    PIN_GPIO15 = 15,    // RTS
    PIN_RST,
    PIN_RES
};

static int getPinHandler(void *data, char *value)
{
    int pin = (int)data;
    int ivalue = 0;
    switch (pin) {
    case PIN_GPIO0:
    case PIN_GPIO2:
    case PIN_GPIO4:
    case PIN_GPIO5:
    case PIN_GPIO12:
    case PIN_GPIO13:
    case PIN_GPIO14:
    case PIN_GPIO15:
        GPIO_DIS_OUTPUT(pin);
        ivalue = GPIO_INPUT_GET(pin);
        break;
    case PIN_RST:
        break;
    case PIN_RES:
        break;
    default:
        return -1;
    }
    os_sprintf(value, "%d", ivalue);
    return 0;
}

static int setPinHandler(void *data, char *value)
{
    int pin = (int)data;
    switch (pin) {
    case PIN_GPIO0:
    case PIN_GPIO2:
    case PIN_GPIO4:
    case PIN_GPIO5:
    case PIN_GPIO12:
    case PIN_GPIO13:
    case PIN_GPIO14:
    case PIN_GPIO15:
        GPIO_OUTPUT_SET(pin, atoi(value));
        break;
    case PIN_RST:
        break;
    case PIN_RES:
        break;
    default:
        return -1;
    }
    return 0;
}

static int intGetHandler(void *data, char *value)
{
    int *pValue = (int *)data;
    os_sprintf(value, "%d", *pValue);
    return 0;
}

static int intSetHandler(void *data, char *value)
{
    int *pValue = (int *)data;
    *pValue = atoi(value);
    return 0;
}

typedef struct {
    char *name;
    int (*getHandler)(void *data, char *value);
    int (*setHandler)(void *data, char *value);
    void *data;
} cmd_def;

static cmd_def vars[] = {
{   "ip-address",       getIPAddress,   setIPAddress,   NULL                        },
{   "pause-time",       intGetHandler,  intSetHandler,  &sscp_pauseTimeMS           },
{   "enable-sscp",      intGetHandler,  intSetHandler,  &flashConfig.enable_sscp    },
{   "baud-rate",        intGetHandler,  setBaudrate,    &uart0_baudRate             },
{   "pin-pgm",          getPinHandler,  setPinHandler,  (void *)0                   },
{   "pin-gpio0",        getPinHandler,  setPinHandler,  (void *)0                   },
{   "pin-dbg",          getPinHandler,  setPinHandler,  (void *)2                   },
{   "pin-gpio2",        getPinHandler,  setPinHandler,  (void *)2                   },
{   "pin-sel",          getPinHandler,  setPinHandler,  (void *)4                   },
{   "pin-gpio4",        getPinHandler,  setPinHandler,  (void *)4                   },
{   "pin-asc",          getPinHandler,  setPinHandler,  (void *)5                   },
{   "pin-gpio5",        getPinHandler,  setPinHandler,  (void *)5                   },
{   "pin-dtr",          getPinHandler,  setPinHandler,  (void *)12                  },
{   "pin-gpio12",       getPinHandler,  setPinHandler,  (void *)12                  },
{   "pin-cts",          getPinHandler,  setPinHandler,  (void *)13                  },
{   "pin-gpio13",       getPinHandler,  setPinHandler,  (void *)13                  },
{   "pin-dio9",         getPinHandler,  setPinHandler,  (void *)14                  },
{   "pin-gpio14",       getPinHandler,  setPinHandler,  (void *)14                  },
{   "pin-rts",          getPinHandler,  setPinHandler,  (void *)15                  },
{   "pin-gpio15",       getPinHandler,  setPinHandler,  (void *)15                  },
{   "pin-rst",          getPinHandler,  setPinHandler,  (void *)PIN_RST             },
{   "pin-res",          getPinHandler,  setPinHandler,  (void *)PIN_RES             },
{   NULL,               NULL,           NULL,           NULL                        }
};

// GET,var
void ICACHE_FLASH_ATTR cmds_do_get(int argc, char *argv[])
{
    char value[128];
    int i;
    
    if (argc != 2) {
        sscp_sendResponse("E,%d", SSCP_ERROR_WRONG_ARGUMENT_COUNT);
        return;
    }
    
    for (i = 0; vars[i].name != NULL; ++i) {
        if (os_strcmp(argv[1], vars[i].name) == 0) {
            if ((*vars[i].getHandler)(vars[i].data, value) == 0)
                sscp_sendResponse("S,%s", value);
            else
                sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
            return;
        }
    }

    sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
}

// SET,var,value
void ICACHE_FLASH_ATTR cmds_do_set(int argc, char *argv[])
{
    int i;
    
    if (argc != 3) {
        sscp_sendResponse("E,%d", SSCP_ERROR_WRONG_ARGUMENT_COUNT);
        return;
    }
    
    for (i = 0; vars[i].name != NULL; ++i) {
        if (os_strcmp(argv[1], vars[i].name) == 0) {
            if ((*vars[i].setHandler)(vars[i].data, argv[2]) == 0)
                sscp_sendResponse("S,0");
            else
                sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
            return;
        }
    }

    sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
}

// POLL
void ICACHE_FLASH_ATTR cmds_do_poll(int argc, char *argv[])
{
    sscp_connection *connection;
    HttpdConnData *connData;
    Websock *ws;
    int i;

    if (argc != 1) {
        sscp_sendResponse("E:%,invalid arguments", SSCP_ERROR_WRONG_ARGUMENT_COUNT);
        return;
    }

    for (i = 0, connection = sscp_connections; i < SSCP_CONNECTION_MAX; ++i, ++connection) {
        if (connection->listener) {
            if (connection->flags & CONNECTION_INIT) {
                connection->flags &= ~CONNECTION_INIT;
                switch (connection->listener->type) {
                case LISTENER_HTTP:
                    connData = (HttpdConnData *)connection->d.http.conn;
                    if (connData) {
                        switch (connData->requestType) {
                        case HTTPD_METHOD_GET:
                            sscp_sendResponse("G:%d,%s", connection->index, connData->url);
                            break;
                        case HTTPD_METHOD_POST:
                            sscp_sendResponse("P:%d,%s", connection->index, connData->url);
                            break;
                        default:
                            sscp_sendResponse("E:0,invalid request type");
                            break;
                        }
                        return;
                    }
                    break;
                case LISTENER_WEBSOCKET:
                    ws = (Websock *)connection->d.ws.ws;
                    if (ws) {
                        sscp_sendResponse("W:%d,%s", connection->index, ws->conn->url);
                        return;
                    }
                    break;
                default:
                    break;
                }
            }
            else if (connection->flags & CONNECTION_RXFULL) {
                sscp_sendResponse("D:%d,%d", connection->index, connection->rxCount);
                return;
            }
        }
    }
    
    sscp_sendResponse("N:0,");
}

// SEND,chan,payload
void ICACHE_FLASH_ATTR cmds_do_send(int argc, char *argv[])
{
    sscp_connection *c;

    if (argc != 3) {
        sscp_sendResponse("E,%d", SSCP_ERROR_WRONG_ARGUMENT_COUNT);
        return;
    }
    
    if (!(c = sscp_get_connection(atoi(argv[1])))) {
        sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
        return;
    }
    
    if (!c->listener) {
        sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
        return;
    }
    
    if (c->flags & CONNECTION_TXFULL) {
        sscp_sendResponse("E,%d", SSCP_ERROR_BUSY);
        return;
    }
    
    switch (c->listener->type) {
    case LISTENER_WEBSOCKET:
        ws_send_helper(c, argc, argv);
        break;
    case LISTENER_TCP:
        tcp_send_helper(c, argc, argv);
        break;
    default:
        sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
        break;
    }
}

// RECV,chan
void ICACHE_FLASH_ATTR cmds_do_recv(int argc, char *argv[])
{
    sscp_connection *connection;

    if (argc != 2) {
        sscp_sendResponse("E,%d", SSCP_ERROR_WRONG_ARGUMENT_COUNT);
        return;
    }

    if (!(connection = sscp_get_connection(atoi(argv[1])))) {
        sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
        return;
    }

    if (!connection->listener) {
        sscp_sendResponse("E,%d", SSCP_ERROR_INVALID_ARGUMENT);
        return;
    }

    if (!(connection->flags & CONNECTION_RXFULL)) {
        sscp_sendResponse("N,0");
        return;
    }

    sscp_sendResponse("S,%d", connection->rxCount);
    sscp_sendPayload(connection->rxBuffer, connection->rxCount);
    connection->flags &= ~CONNECTION_RXFULL;
}

int ICACHE_FLASH_ATTR cgiPropSetting(HttpdConnData *connData)
{
    char name[128], value[128];
    cmd_def *def = NULL;
    int i;
    
    // check for the cleanup call
    if (connData->conn == NULL)
        return HTTPD_CGI_DONE;

    if (httpdFindArg(connData->getArgs, "name", name, sizeof(name)) < 0) {
        httpdSendResponse(connData, 400, "Missing name argument\r\n", -1);
        return HTTPD_CGI_DONE;
    }

    for (i = 0; vars[i].name != NULL; ++i) {
        if (os_strcmp(name, vars[i].name) == 0) {
            def = &vars[i];
            break;
        }
    }
    
    if (!def) {
        httpdSendResponse(connData, 400, "Unknown setting\r\n", -1);
        return HTTPD_CGI_DONE;
    }

    // check for GET
    if (connData->requestType == HTTPD_METHOD_GET) {
        if ((*def->getHandler)(def->data, value) != 0) {
            httpdSendResponse(connData, 400, "Get setting failed\r\n", -1);
            return HTTPD_CGI_DONE;
        }
    }

    // only other option is POST
    else {
        if (httpdFindArg(connData->getArgs, "value", value, sizeof(value)) < 0) {
            httpdSendResponse(connData, 400, "Missing value argument\r\n", -1);
            return HTTPD_CGI_DONE;
        }
        if ((*def->setHandler)(def->data, value) != 0) {
            httpdSendResponse(connData, 400, "Set setting failed\r\n", -1);
            return HTTPD_CGI_DONE;
        }
        os_strcpy(value, "");
    }

    httpdStartResponse(connData, 200);
    httpdEndHeaders(connData);
    httpdSend(connData, value, -1);

    return HTTPD_CGI_DONE;
}
