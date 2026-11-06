// Pico 2 W Wi-Fi -> CAN gateway
// Receives JSON telemetry over UDP and forwards x, y, angle of marker 17
// on the CAN bus:
//   ID 0x200, 8 bytes: x (float), y (float)   in meters
//   ID 0x201, 4 bytes: angle (int32)          in degrees
//
// The MCP2515 loopback self-test is disabled on purpose (see previous notes).

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "pico/stdlib.h"
#include "mcp2515.h"

#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

#define TARGET_MARKER_ID 17

static const char *WIFI_SSID = "embedded";
static const char *WIFI_PASSWORD = "12345678";
static const uint16_t TELEMETRY_PORT = 5000;

static bool send_pose(float x, float y, int32_t angle) {
    can_frame_t f1 = { .id = 0x200, .dlc = 8 };
    memcpy(&f1.data[0], &x, sizeof(x));
    memcpy(&f1.data[4], &y, sizeof(y));

    can_frame_t f2 = { .id = 0x201, .dlc = 4 };
    memcpy(&f2.data[0], &angle, sizeof(angle));

    bool ok1 = mcp2515_send_message(&f1);
    sleep_ms(2);   // let TXB0 clear before loading the next frame
    bool ok2 = mcp2515_send_message(&f2);

    return ok1 && ok2;
}

// Reads "key": <number> starting at s. Returns false if the key is missing.
static bool get_number(const char *s, const char *key, float *out) {
    char pat[24];
    snprintf(pat, sizeof(pat), "\"%s\":", key);

    const char *k = strstr(s, pat);
    if (!k) return false;

    return sscanf(k + strlen(pat), " %f", out) == 1;
}

static void on_udp_packet(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                          const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb; (void)addr; (void)port;

    if (p == NULL) return;

    char json_str[512];
    size_t n = p->tot_len < sizeof(json_str) - 1 ? p->tot_len : sizeof(json_str) - 1;
    pbuf_copy_partial(p, json_str, n, 0);
    json_str[n] = '\0';
    pbuf_free(p);

    // Walk through every marker in the "markers" array.
    // Each marker starts with "id", followed by its own x, y and angle.
    const char *cur = strstr(json_str, "\"markers\"");
    if (!cur) return;

    while ((cur = strstr(cur, "\"id\":")) != NULL) {
        float id_f;
        if (!get_number(cur, "id", &id_f)) break;

        int id = (int)lroundf(id_f);

        if (id == TARGET_MARKER_ID) {
            float x, y, angle_f;
            if (get_number(cur, "x", &x) &&
                get_number(cur, "y", &y) &&
                get_number(cur, "angle", &angle_f)) {

                int32_t angle = (int32_t)lroundf(angle_f);
                printf("Marker %d -> X: %.3f | Y: %.3f | Angle: %ld\n",
                       id, x, y, (long)angle);

                printf(send_pose(x, y, angle) ? "TX OK\n" : "TX failed\n");
            } else {
                printf("Marker %d missing field(s)\n", id);
            }
            return;   // only one marker 17 per packet is expected
        }

        cur += 5;   // move past this "id": to find the next marker
    }
    // No marker 17 in this packet: nothing is sent.
}

int main(void) {
    stdio_init_all();
    sleep_ms(3000);

    printf("\n--- Pico 2 W Wi-Fi -> CAN gateway (marker %d) ---\n", TARGET_MARKER_ID);

    // CAN
    mcp2515_init();
    mcp2515_set_bitrate_500kbps_8mhz();
    mcp2515_set_mode(MCP2515_MODE_NORMAL);
    printf("MCP2515 in NORMAL mode.\n");

    // Wi-Fi
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                              CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Wi-Fi connect failed, retrying...\n");
    }
    printf("Connected. IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

    cyw43_wifi_pm(&cyw43_state, CYW43_NO_POWERSAVE_MODE);

    struct udp_pcb *pcb = udp_new();
    udp_bind(pcb, IP_ADDR_ANY, TELEMETRY_PORT);
    udp_recv(pcb, on_udp_packet, NULL);
    printf("Listening for UDP on port %u\n", TELEMETRY_PORT);

    while (true) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1));
    }
}