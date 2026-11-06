#include <stdio.h>
#include "pico/stdlib.h"
#include "mcp2515.h"

#include <string.h>
#include "pico/cyw43_arch.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"


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
    bool ok2 = mcp2515_send_message(&f2);
    return ok1 && ok2;
}

static void on_udp_packet(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                          const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb; (void)addr; (void)port;

    if (p == NULL) return;

    char json_str[256];
    size_t n = p->tot_len < sizeof(json_str) - 1 ? p->tot_len : sizeof(json_str) - 1;
    pbuf_copy_partial(p, json_str, n, 0);
    json_str[n] = '\0';
    pbuf_free(p);   

    float x = 0.0f, y = 0.0f;
    int angle = 0;
    char *ptr;
    bool got_x = false, got_y = false, got_angle = false;

    ptr = strstr(json_str, "\"x\":");
    if (ptr) got_x = (sscanf(ptr, "\"x\": %f", &x) == 1);

    ptr = strstr(json_str, "\"y\":");
    if (ptr) got_y = (sscanf(ptr, "\"y\": %f", &y) == 1);

    ptr = strstr(json_str, "\"angle\":");
    if (ptr) got_angle = (sscanf(ptr, "\"angle\": %d", &angle) == 1);

    if (!(got_x && got_y && got_angle)) {
        printf("Bad packet, missing field(s): %s\n", json_str);
        return;   // don't send incomplete data to the CAN bus
    }

       printf("Extracted -> X: %.3f | Y: %.3f | Angle: %d\n", x, y, angle);

    if (send_pose(x, y, (int32_t)angle)) {
        printf("TX OK\n");
    } else {
        printf("TX failed\n");
    }
}

static bool run_loopback_self_test(void) {
    printf("Running MCP2515 loopback self-test...\n");
    mcp2515_set_mode(MCP2515_MODE_LOOPBACK);

    can_frame_t tx_frame = {
        .id = 0x123,
        .dlc = 4,
        .data = {0xDE, 0xAD, 0xBE, 0xEF}
    };

    if (!mcp2515_send_message(&tx_frame)) {
        printf("Loopback TX failed\n");
        return false;
    }

    sleep_ms(50);

    can_frame_t rx_frame;
    if (!mcp2515_read_message(&rx_frame)) {
        printf("Loopback RX failed: no message received\n");
        return false;
    }

    bool ok = (rx_frame.id == tx_frame.id) && (rx_frame.dlc == tx_frame.dlc);
    for (int i = 0; i < rx_frame.dlc && ok; i++) {
        if (rx_frame.data[i] != tx_frame.data[i]) ok = false;
    }

    printf("Loopback result: ID=0x%03X DLC=%d Data=", (unsigned)rx_frame.id, rx_frame.dlc);
    for (int i = 0; i < rx_frame.dlc; i++) printf("%02X ", rx_frame.data[i]);
    printf("-> %s\n", ok ? "PASS" : "FAIL");

    return ok;
}


int main(void) {
	stdio_init_all();
    sleep_ms(3000);

    printf("\n--- Pico 2 W + WIFI demo ---\n");

    //CAN------------------------------
    mcp2515_init();
    mcp2515_set_bitrate_500kbps_8mhz();

    if (!run_loopback_self_test()) {
        printf("Self-test FAILED. Check wiring/crystal frequency before trusting the bus.\n");
    } else {
        printf("Self-test PASSED. Switching to NORMAL mode.\n");
    }

    mcp2515_set_mode(MCP2515_MODE_NORMAL);

    //WIFI------------------------------
 
	if (cyw43_arch_init()) {
    	printf("Wi-Fi init failed\n");
    	return -1;
	}
	cyw43_arch_enable_sta_mode();
 
	while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           	CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
    	printf("Wi-Fi connect failed, retrying...\n");
	}
	printf("Connected\n");
 

	// Equivalent of Arduino's WiFi.noLowPowerMode(): without this, the
	// radio buffers broadcast packets until periodic wake-ups instead of
	// receiving them promptly, causing multi-second delays.
	cyw43_wifi_pm(&cyw43_state, CYW43_NO_POWERSAVE_MODE);
	struct udp_pcb *pcb = udp_new();
	udp_bind(pcb, IP_ADDR_ANY, TELEMETRY_PORT);
	udp_recv(pcb, on_udp_packet, NULL);

	while (true) {
    	cyw43_arch_poll();
    	cyw43_arch_wait_for_work_until(make_timeout_time_ms(1));
	}
}
