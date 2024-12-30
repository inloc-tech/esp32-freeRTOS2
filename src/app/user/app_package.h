
#ifndef APP_PACKAGE_H
#define APP_PACKAGE_H

/////////////////////////////////////////////////////////////////////
//                                                                 //
//                            APP Version    											 //
//                                                                 //
/////////////////////////////////////////////////////////////////////

#define APP_VERSION        			 	"1.0.0"

/////////////////////////////////////////////////////////////////////
//                                                                 //
// FW MODEL       																						 		 //
//                                                                 //
/////////////////////////////////////////////////////////////////////

/* supported Models by our cloud
* - devices.inloc.cloud
* If you chose a different model you have to develop your own web
* interface or request support. Contact developer in that case:
* - lucas.ua.eet@gmail.com
*/
#define FW_MODEL                  "SNIFFER"

/////////////////////////////////////////////////////////////////////
//                                                                 //
//                         APP PROCESS		                       //
//                                                                 //
/////////////////////////////////////////////////////////////////////

// select just one option
//#define FAST_APP // App runing on main thread
//#define THREAD_APP // 1 dedicated thread for app
#define SYNCHED_APP // App running on core thread

/////////////////////////////////////////////////////////////////////
//                                                                 //
//                         HARDWARE SETTINGS                       //
//                                                                 //
/////////////////////////////////////////////////////////////////////

//#define ENABLE_JS // JavaScript for user runtime code
#define EXT_SERIAL_COMM // uart2 assigned to Serial1
//#define ENABLE_RS485 // enable rs485 module on Serial1
//#define DIGITAL_COUNTERS

/////////////////////////////////////////////////////////////////////
//                                                                 //
//                         IOS MAPPING                             //
//                                                                 //
/////////////////////////////////////////////////////////////////////

#define SERIAL1_GPIO_RX 16
#define SERIAL1_GPIO_TX 17
#define SERIAL1_GPIO_RTS 13

/////////////////////////////////////////////////////////////////////
//                                                                 //
//                         DEBUG                             	   //
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define DEBUG_SNIFFER

/////////////////////////////////////////////////////////////////////
//                                                                 //
//                   Libraries macros                              //
//                                                                 //
/////////////////////////////////////////////////////////////////////
#define MQTT_TX_QUEUE_SIZE 20

#endif
