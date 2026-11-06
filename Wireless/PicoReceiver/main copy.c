#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <pololu_zumo_2040_robot.h>

#include "pico/stdlib.h"
#include "mcp2515.h"

#define CAN_TELEMETRY_ID 0x500

#define RIGHT_FORWARD_SPEED 1180
#define LEFT_FORWARD_SPEED 1280

static float pos_x = 0.0f;  
static float pos_y = 0.0f;   
static float angle = 0.0f;   
static float steps; 
bool first_time = true;


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

static void handle_can_frame(const can_frame_t *f) {
    if (f->id != CAN_TELEMETRY_ID || f->dlc != 6) return;   

    int16_t x_mm, y_mm, a_tdeg;
    memcpy(&x_mm,   &f->data[0], 2);
    memcpy(&y_mm,   &f->data[2], 2);
    memcpy(&a_tdeg, &f->data[4], 2);

    pos_x = x_mm / 1000.0f;
    pos_y = y_mm / 1000.0f;
    angle = a_tdeg / 10.0f;

    printf("x=%.3f m  y=%.3f m  angle=%.1f deg\n", pos_x, pos_y, angle);

    if( first_time ){
        steps = pos_y;
        first_time = false;
    }

    if(pos_y <= steps + 0.50){
        motors_set_speeds(LEFT_FORWARD_SPEED, RIGHT_FORWARD_SPEED); 
    }else{
        motors_set_speeds(0, 0);
    }
}


int main(void) {
    stdio_init_all();
    sleep_ms(8000); // give the USB serial connection time to enumerate

    printf("\n--- Pico + MCP2515 CAN demo ---\n");
    mcp2515_init();
    mcp2515_set_bitrate_500kbps_8mhz();

    if (!run_loopback_self_test()) {
        printf("Self-test FAILED. Check wiring/crystal frequency before trusting the bus.\n");
    } else {
        printf("Self-test PASSED. Switching to NORMAL mode.\n");
    }

    mcp2515_set_mode(MCP2515_MODE_NORMAL);

    while (true) {
        
        if (mcp2515_message_available()) {
            can_frame_t rx;
            if (mcp2515_read_message(&rx)) {
                handle_can_frame(&rx);
            }
        }

        sleep_ms(10);
    }
}
