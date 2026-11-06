#include <stdio.h>
#include <stdlib.h>
#include <pololu_zumo_2040_robot.h>
#include "pico/stdlib.h"
#include "encoder.h"
#define COUNTS_PER_METER -7903
#define BASE_SPEED        2200
#define KP                2
#define COUNTS_PER_90_DEGREES 650

void drive_straight_counts(int32_t target_counts) {
    encoder_reset();
    while (true) {
        int32_t left, right= 0;
        encoder_get_counts(&left, &right);
        left = -1*left;
        right = -1*right;
        printf("L = %ld R = %ld\n",(long)left,(long)right);
        int32_t distance = (left + right) / 2;
        if (distance >= target_counts) {
            break;
        }
        int32_t error = left - right;
        int32_t correction = KP * error;
        int32_t left_speed = BASE_SPEED - correction;
        int32_t right_speed = BASE_SPEED + correction;
        /* Limit correction
         */
        if (left_speed > 3000)
            left_speed = 3000;
        if (left_speed < 1000)
            left_speed = 1000;
        if (right_speed > 3000)
            right_speed = 3000;
        if (right_speed < 1000)
            right_speed = 1000;
        motors_set_speeds(left_speed, right_speed);
        sleep_ms(5);
    }
    motors_set_speeds(0, 0);
    sleep_ms(300);
}

void drive_one_meter(int32_t target) {
    encoder_reset();

    int32_t left;
    int32_t right;
    while (true) {
        encoder_get_counts(&left, &right);
        left = -1*left;
        right = -1*right;
        int32_t average = (left + right) / 2;
        if (average >= target)
            break;
        int32_t remaining = target - average;
        int base_speed;
        if (remaining > 1000)
            base_speed = 2200;
        else if (remaining > 400)
            base_speed = 1500;
        else
            base_speed = 800;
        int32_t error = left - right;
        int32_t correction =  KP * error;
        motors_set_speeds(base_speed-correction, base_speed + correction);
        sleep_ms(5);
    }
    motors_set_speeds(0, 0);
    sleep_ms(300);
}

#define TURN_SPEED 1500
void turn_right_90(int32_t target_counts) {
    encoder_reset();
    while (true) {
        int32_t left, right= 0;
        encoder_get_counts(&left, &right);
        left = -1*left;
        right = -1*right;
        printf("L = %ld R = %ld\n",(long)left,(long)right);
        int32_t rotation = (abs(left) + abs(right)) / 2;
        if (rotation >= target_counts) {
            break;
        }
        motors_set_speeds(TURN_SPEED, -TURN_SPEED);
        sleep_ms(5);
    }
    motors_set_speeds(0, 0);
    sleep_ms(300);
}


void turn_right_90_precise(int32_t target) {
    encoder_reset();

    int32_t left;
    int32_t right;
    while (true) {
        encoder_get_counts(&left, &right);

        int32_t count = (abs(left) + abs(right)) / 2;
        int32_t remaining = target - count;
        printf("L = %ld R = %ld\n",(long)left,(long)right);
        if (remaining <= 0)
            break;
        int speed;
        if (remaining > 300)
            speed = 1800;
        else if (remaining > 100)
            speed = 1600;
        else
            speed = 1000;
        motors_set_speeds(speed,-speed);
        sleep_ms(5);
    }
    motors_set_speeds(0, 0);
    sleep_ms(300);
}

void move_forward_correction(int32_t counts) {
    while (true)
    {
        int32_t left;
        int32_t right;
        encoder_get_counts(&left, &right);
        int32_t average = (abs(left) + abs(right)) / 2;
        if (average >= counts)
            break;
        motors_set_speeds(700, 700);
        sleep_ms(5);
    }
    motors_set_speeds(0, 0);
}

void turn_correction(int32_t target) {

    int32_t left;
    int32_t right;
    while (true) {
        encoder_get_counts(&left, &right);

        int32_t count = (abs(left) + abs(right)) / 2;
        int32_t remaining = target - count;
        printf("L = %ld R = %ld\n",(long)left,(long)right);
        if (remaining <= 0)
            break;
        int speed;
            speed = 1000;
        motors_set_speeds(speed,-speed);
        sleep_ms(5);
    }
    motors_set_speeds(0, 0);
    sleep_ms(300);
}

int main(void) {
    stdio_init_all();
    motors_init();
    encoder_init();
    encoder_reset();
    sleep_ms(2000);

    for (int i = 0; i < 4; i++) {
        sleep_ms(2000);
        drive_one_meter(abs(COUNTS_PER_METER) - 100); 
        sleep_ms(2000);
        move_forward_correction(abs(COUNTS_PER_METER));
        sleep_ms(2000);
        turn_right_90_precise(COUNTS_PER_90_DEGREES - 100);
        sleep_ms(2000);
        turn_correction(COUNTS_PER_90_DEGREES);
    }
    

}