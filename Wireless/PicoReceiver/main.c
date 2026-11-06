// Zumo 2040 CAN receiver
// - ID 0x200 (8 bytes): x, y as float, meters
// - ID 0x201 (4 bytes): angle as int32, degrees
// - Drives forward until y has changed 0.50 m from the first reading
// - Sends an "I'm alive" heartbeat (ID 0x600) every second
// - Prints all received and sent data in decimal

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <pololu_zumo_2040_robot.h>

#include "pico/stdlib.h"
#include "mcp2515.h"

#define CAN_POS_ID          0x200
#define CAN_ANGLE_ID        0x201
#define CAN_HEARTBEAT_ID    0x600
#define HEARTBEAT_PERIOD_MS 1000

#define RIGHT_FORWARD_SPEED 1180
#define LEFT_FORWARD_SPEED  1280
#define TARGET_DISTANCE_M   0.50f

static float   pos_x = 0.0f;   // meters
static float   pos_y = 0.0f;   // meters
static int32_t angle = 0;      // degrees
static float   start_y;
static bool    first_time = true;

static void handle_can_frame(const can_frame_t *f) {
    if (f->id == CAN_ANGLE_ID && f->dlc == 4) {
        memcpy(&angle, &f->data[0], 4);
        printf("RX ID=0x%03X  angle=%ld deg\n", (unsigned)f->id, (long)angle);
        return;
    }

    if (f->id == CAN_POS_ID && f->dlc == 8) {
        memcpy(&pos_x, &f->data[0], 4);
        memcpy(&pos_y, &f->data[4], 4);
        printf("RX ID=0x%03X  x=%.3f m  y=%.3f m\n", (unsigned)f->id, pos_x, pos_y);

        if (first_time) {
            start_y = pos_y;
            first_time = false;
        }

        float moved = fabsf(pos_y - start_y);
        if (moved < TARGET_DISTANCE_M) {
            motors_set_speeds(LEFT_FORWARD_SPEED, RIGHT_FORWARD_SPEED);
        } else {
            motors_set_speeds(0, 0);
            printf("   target reached (moved %.3f m), motors stopped\n", moved);
        }
        return;
    }

    // Any other frame: print its bytes in decimal
    printf("RX ID=0x%03X  DLC=%u  Data=", (unsigned)f->id, f->dlc);
    for (int i = 0; i < f->dlc; i++) printf("%u ", f->data[i]);
    printf("\n");
}

static void send_heartbeat(uint16_t counter) {
    can_frame_t hb = {
        .id = CAN_HEARTBEAT_ID,
        .dlc = 2,
        .data = { (uint8_t)(counter >> 8), (uint8_t)(counter & 0xFF) }
    };

    if (mcp2515_send_message(&hb)) {
        printf("TX ID=0x%03X  alive counter=%u\n", (unsigned)hb.id, (unsigned)counter);
    } else {
        printf("TX failed\n");
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(3000);

    printf("\n--- Zumo CAN receiver ---\n");

    // Same order as the working diagnostic build: motors first, then CAN
    motors_init();
    motors_set_speeds(0, 0);

    mcp2515_init();
    mcp2515_set_bitrate_500kbps_8mhz();
    mcp2515_set_mode(MCP2515_MODE_NORMAL);

    uint16_t hb_counter = 0;
    absolute_time_t next_hb = make_timeout_time_ms(HEARTBEAT_PERIOD_MS);

    while (true) {
        while (mcp2515_message_available()) {
            can_frame_t rx;
            if (!mcp2515_read_message(&rx)) break;
            handle_can_frame(&rx);
        }

        if (absolute_time_diff_us(get_absolute_time(), next_hb) <= 0) {
            send_heartbeat(hb_counter++);
            next_hb = make_timeout_time_ms(HEARTBEAT_PERIOD_MS);
        }

        sleep_ms(1);
    }
}