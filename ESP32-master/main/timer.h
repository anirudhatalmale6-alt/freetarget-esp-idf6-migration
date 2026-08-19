/*----------------------------------------------------------------
 *
 * gpio.h
 *
 * Header file for GPIO functions
 *
 *---------------------------------------------------------------*/
#ifndef _TIMER_H_
#define _TIMER_H_

/*
 * Variables
 */
extern time_count_t ring_timer; // Let the ring on the backstop end
extern time_count_t time_to_go; // Time remaining in event in seconds

/*
 * function Prototypes
 */
void         freeETarget_timer_init(void);                                                         // Initialize the timers
void         freeETarget_timer_pause(void);                                                        // Stop the timer
void         freeETarget_timer_start(void);                                                        // Start the timer
// TODO(IDF6): prototype corrected to match the definition in timer.c.
// It was:  int ft_timer_new(time_count_t *timer_new, long duration, void *(callback)(), char *name);
// Two problems. The duration argument is a time_count_t in the definition,
// not a long. And "void *(callback)()" declares a pointer to a function
// returning void* and taking unspecified arguments - under C23 an empty
// parameter list means (void), so it no longer silently matches. The
// timers[] struct in timer.c declares the field as void (*)(void) and calls
// it that way, so this is what it was always meant to say.
int          ft_timer_new(time_count_t *timer_new, time_count_t duration, void (*callback)(void), char *name); // Start a new timer in ms
int          ft_timer_delete(time_count_t *timer);                                                 // Stop a running timer
void         freeETarget_synchronous(void *pvParameters);                                          // Synchronou scheduler
void         freeETarget_timers(void *pvParameters);                                               // Update the free running timers
void         show_time(void);                                                                      // Show the current time
// TODO(IDF6): return types were time_count_t, which is "typedef volatile
// int32_t". A volatile qualifier on a RETURN type is meaningless - the
// compiler discards it - and GCC 15 now makes that -Werror=ignored-qualifiers.
// Changed to int32_t. The typedef itself is untouched, so variables declared
// time_count_t are still volatile, which is the part that matters.
int32_t      run_time_seconds(void);                                                               // Show how long we have been running for
int32_t      run_time_ms(void);    // Show how long we have been running for in ms
void         reset_run_time(void); // Reset the clock back to zero
void         show_timers(void);    // Show the current timers

/*
 *  Definitions
 */

#endif
