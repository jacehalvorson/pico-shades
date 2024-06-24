#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

#define TIMEZONE_OFFSET_FROM_UTC_SEC (-5 * 3600)

// #define DEBUG

#ifdef DEBUG
    #define debug_printf(...) printf(__VA_ARGS__)
#else
    #define debug_printf(...)
#endif

// Called with results of operation
static void ntp_result(NTP_T* state, int status, time_t *result);
// Called with results of failure
static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
// Make an NTP request
static void ntp_request(NTP_T *state);
// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
// Perform initialization
static NTP_T* ntp_init(void);

// Sets RTC to current time
void set_rtc_time(void);