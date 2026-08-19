/*----------------------------------------------------------------
 *
 * NTP.h
 *
 * Header file for Network Time Protocol
 *
 *---------------------------------------------------------------*/
#ifndef _NTP_H_
#define _NTP_H_

/*
 * Variables
 */
// TODO(IDF6): return types were time_count_64_t, which is "typedef volatile
// int64_t". A volatile qualifier on a RETURN type is meaningless - the
// compiler discards it - and GCC 15 makes that -Werror=ignored-qualifiers.
// Changed to int64_t. The typedef itself is untouched, so variables declared
// time_count_64_t are still volatile, which is the part that matters.
int64_t         NTP_time_us(void); // Coordinated time in us
int64_t         NTP_time_ms(void); // Coordinated time in ms
int64_t         NTP_time_s(void);  // Coordinated time in seconds

/*
 * function Prototypes
 */
bool NTP_ttg(void);    // TRUE if we need an NTP refresh
void NTP_server(void); // Originating signal to start time sync
void NTP_client(void); // Receiving signal to syncronize time
void NTP_ask(void);    // Begin to calculate the loop time
void NTP_test(void); // Keep asking for NTP time 

/*
 *  Definitions
 */
#define _NTP_MASTER_ "NTP_ASK"    // Ask for a time sync
#define _NTP_CLIENT_ "NTP_CLIENT" // Synchronization message from trace
#define _NTP_SERVER_ "NTP_SERVER" // Work out the loop time
#endif
