#include <Canbus.h>
#include <SPI.h>

#define _VERSION_STRING "can_usb_0.1"
#define CAN_FILTER_max 10

const byte ledPin = 7;
volatile byte state = LOW;
String cmd;
char sbuf[32];

struct can_filter_struct {
    unsigned long can_rx_id;       
} CAN_FILTER[CAN_FILTER_max];

//********************************Setup Loop*********************************//

void setup(){
  Serial.begin(230400); 
  // make SPI as fast as possible (8Mhz)
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  // setup ledPin
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin+1, OUTPUT);
  // init CANBUS
  Canbus.init(CANSPEED_500);
  Canbus.reset_filters();
  Canbus.add_filter(0x400);
  Canbus.add_filter(0x401);
}

void blink() {
  state = !state;
}

// helper function - count number of "c" within "*msg"
unsigned char count_chr(char *msg, unsigned char c)
{
    unsigned char i,cnt=0;

    if (!msg) return(0);
    for (i=0;i<strlen(msg);i++)
    {
        if (msg[i]==c) cnt++;
    }
    return(cnt);
}

//////////////////////////////////////////////////////////////////////
// parse "VERSION" and show version information
// calls: 
// format:
//          $VER
//////////////////////////////////////////////////////////////////////
void parse_version(void)
{
  sprintf(sbuf, "$VER,OK,%s\r\n", _VERSION_STRING);
  Serial.print(sbuf);
}

//////////////////////////////////////////////////////////////////////
// add CAN Filter
// calls:
// format:
//          $FA,<ID>
//////////////////////////////////////////////////////////////////////
void parse_filter_add(char *msg)
{
    unsigned char  c_count, i;     // , count
    unsigned char  state=0;
    char  *b;
    char *ptr;   
    unsigned int   val_f;
    
    // check format   
    c_count=count_chr(msg, ',');
    if ((c_count != 1))
    {      
        Serial.println("$FA,FAIL");
        return;
    }
    
    // get <ID>
    b=strchr(msg,',')+1;
    val_f = strtol(b, &ptr, 16);

    state = Canbus.add_filter(val_f);
    if (state == 0)
     Serial.println("$FA,FAIL,EXISTING");
    else if (state == 1)
     Serial.println("$FA,OK");
    else
     Serial.println("$FA,FAIL,NOSLOTLEFT");
    return;    
}

//////////////////////////////////////////////////////////////////////
// del CAN Filter
// calls:
// format:
//          $FD,<ID>
//////////////////////////////////////////////////////////////////////
void parse_filter_del(char *msg)
{
    unsigned char  c_count, i;     // , count
    unsigned char  state=0;
    char  *b;
    char *ptr;   
    unsigned int   val_f;
    
     // check format    
    c_count=count_chr(msg, ',');
    if ((c_count != 1))
    {        
        Serial.println("$FD,FAIL");
        return;
    }    
    // get <ID>
    b=strchr(msg,',')+1;
    val_f = strtol(b, &ptr, 16);

    state = Canbus.del_filter(val_f);
    if (state == 1)
     Serial.println("$FD,OK");
    else
     Serial.println("$FD,FAIL,NOSUCHFILTER");
    return;
}

//////////////////////////////////////////////////////////////////////
// parse "CAN send message" and send CAN frame
// format:
//          $S,<len>,<id>,<byte0>,...<byte7>    (hex values without 0x)
//          $S,1,22,33
//          $S,8,2,1,2,3,4,5,6,7,8
//          $S,8,7DF,1,2,3,4,5,6,7,8
//          $S,8,2,A,B,C,D,E,F,AA,BB
//          $S,4,52,0,0,0,0
//          $S,4,50,1,2,3,4
//          $S,1,100,2
//          $S,4,53,1,2,3,4
//          $S,1,53,1
//////////////////////////////////////////////////////////////////////
void parse_send(char *msg)
{
    unsigned char  c_count,i;     // , count
    char  *b;
    char *ptr;
    unsigned long   v;
    unsigned int    can_id;
    unsigned char   can_len;
    unsigned char   can_data[8];

    // check format 
    c_count=count_chr(msg, ',');    
    if ((c_count < 3) || (c_count> 10))
    {       
        Serial.println("$S,FAIL");
        return;
    }
    // get len
    b=strchr(msg,',')+1;    
    can_len = strtol(b, &ptr, 16);  
    if (can_len<1 || can_len>8 || can_len!=c_count-2)
    {        
        Serial.println("$S,FAIL");
        return;
    }
    // get id
    b=strchr(b,',')+1;
    can_id = strtol(b, &ptr, 16);  
    if (can_id>2048)
    {        
        Serial.println("$S,FAIL");
        return;
    }
    // get data bytes  
    for (i=0;i<can_len;i++)
    {
        b=strchr(b,',')+1;
        can_data[i]=strtol(b, &ptr, 16);     
    } 
    // send CAN message
    Canbus.tx_std(can_id, can_len, can_data);
 // if (Canbus.tx_std(can_id, can_len, can_data))        
  //    Serial.println("$S,OK");
  //  else
  //    Serial.println("$S,FAIL");    
}

/*
 * Main Loop
 */
void loop()
{       
      while (Serial.available() > 0)
      {
        cmd = Serial.readStringUntil('\n');
        if (cmd.startsWith("$VER")) parse_version();
        if (cmd.startsWith("$FA"))  parse_filter_add((char*)cmd.c_str());
        if (cmd.startsWith("$FD"))  parse_filter_del((char*)cmd.c_str());        
        if (cmd.startsWith("$S"))   parse_send((char*)cmd.c_str());
      }
      /*
       *  pass any pending received messages onto serial
       */
      if (Canbus.show_buffers() >0) blink();
      digitalWrite(ledPin, state);
}

