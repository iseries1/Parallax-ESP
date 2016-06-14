/*
  Robot test program
*/
#include "simpletools.h"
#include "abdrive.h"
#include "ping.h"
#include "cmd.h"

// uncomment this if the wifi module is on pins other than 31/30
//#define SEPARATE_WIFI_PINS

#ifdef SEPARATE_WIFI_PINS
#define WIFI_RX     9
#define WIFI_TX     8
#else
#define WIFI_RX     31
#define WIFI_TX     30
#endif

#define PING_PIN    10

#define DEBUG

int wheelLeft;
int wheelRight;

void init_robot(void);
int process_robot_command(int whichWay);            
void set_robot_speed(int left, int right);

#define LONG_REPLY  "This is a very long reply that should be broken into multiple chunks to test REPLY followed by SEND.\r\n"

int main(void)
{    
    int listenChan;
    
    cmd_init(WIFI_RX, WIFI_TX, 31, 30);

    init_robot();
    
//    request("SET:cmd-pause-time,5");
    nrequest(SSCP_TKN_SET, "cmd-pause-time,5");
    waitFor(SSCP_PREFIX "=S,0\r");

    request("LISTEN:HTTP,/robot*");
    waitFor(SSCP_PREFIX "=S,^d\r", &listenChan);
    
    for (;;) {
        char url[128], arg[128];
        int type, chan, size;

        waitcnt(CNT + CLKFREQ/4);

        request("POLL");
        waitFor(SSCP_PREFIX "=^c,^i,^i\r", &type, &chan, &size);
        if (type != 'N')
            dprint(debug, "Got %c: chan %d, size %d\n", type, chan, size);
        
        switch (type) {
        case 'P':
            request("PATH:%d", chan);
            waitFor(SSCP_PREFIX "=S,^s\r", url, sizeof(url));
            dprint(debug, "%d: path '%s'\n", chan, url);
            if (strcmp(url, "/robot") == 0) {
                request("ARG:%d,gto", chan);
                waitFor(SSCP_PREFIX "=S,^s\r", arg, sizeof(arg));
                dprint(debug, "gto='%s'\n", arg);
                if (process_robot_command(arg[0]) != 0)
                    dprint(debug, "Unknown robot command: '%c'\n", arg[0]);
                reply(chan, 200, "");
            }
            else {
                dprint(debug, "Unknown POST URL\n");
                reply(chan, 404, "unknown");
            }
            break;
        case 'G':
            request("PATH:%d", chan);
            waitFor(SSCP_PREFIX "=S,^s\r", url, sizeof(url));
            dprint(debug, "%d: path '%s'\n", chan, url);
            if (strcmp(url, "/robot-ping") == 0) {
                sprintf(arg, "%d", ping_cm(PING_PIN));
                reply(chan, 200, arg);
            }
            else if (strcmp(url, "/robot-test") == 0) {
                reply(chan, 200, LONG_REPLY);
            }
            else {
                dprint(debug, "Unknown GET URL\n");
                reply(chan, 404, "unknown");
            }
            break;
        case 'N':
            break;
        default:
            dprint(debug, "unknown response: '%c' 0x%02x\n", type, type);
            break;
        }
    }
    
    return 0;
}

void init_robot(void)
{
  wheelLeft = wheelRight = 0;
  high(26);
  set_robot_speed(wheelLeft, wheelRight);
}

int process_robot_command(int whichWay)            
{ 
  toggle(26);
      
  switch (whichWay) {
  
  case 'F': // forward
    #ifdef DEBUG
      dprint(debug, "Forward\n");
    #endif
    if (wheelLeft > wheelRight)
      wheelRight = wheelLeft;
    else if (wheelLeft < wheelRight) 
      wheelLeft = wheelRight;
    else {           
      wheelLeft = wheelLeft + 32;
      wheelRight = wheelRight + 32;
    }      
    break;    
    
  case 'R': // right
    #ifdef DEBUG
      dprint(debug, "Right\n");
    #endif
    wheelLeft = wheelLeft + 16;
    wheelRight = wheelRight - 16;
    break;
    
  case 'L': // left
    #ifdef DEBUG
      dprint(debug, "Left\n");
    #endif
    wheelLeft = wheelLeft - 16;
    wheelRight = wheelRight + 16;
    break;
    
  case 'B': // reverse
    #ifdef DEBUG
      dprint(debug, "Reverse\n");
    #endif
    if(wheelLeft < wheelRight)
      wheelRight = wheelLeft;
    else if (wheelLeft > wheelRight) 
      wheelLeft = wheelRight;
    else {           
      wheelLeft = wheelLeft - 32;
      wheelRight = wheelRight - 32;
    }
    break;  
        
  case 'S': // stop
    #ifdef DEBUG
      dprint(debug, "Stop\n");
    #endif
    wheelLeft = 0;
    wheelRight = 0;
    break;
    
  default:  // unknown request
    return -1;
  }    
  
  if (wheelLeft > 128) wheelLeft = 128;
  if (wheelLeft < -128) wheelLeft = -128;
  if (wheelRight > 128) wheelRight = 128;
  if (wheelRight < -128) wheelRight = -128;
  
  set_robot_speed(wheelLeft, wheelRight);
    
  return 0;
}

void set_robot_speed(int left, int right)
{  
  #ifdef DEBUG
    dprint(debug, "L %d, R %d\n", wheelLeft, wheelRight);
  #endif
  
  wheelLeft = left;
  wheelRight = right;
  drive_speed(wheelLeft, wheelRight);
}
