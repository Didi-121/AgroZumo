#include <stdio.h>
#include <pololu_zumo_2040_robot.h>
#include "pico/stdlib.h"
#define RIGHT_FORWARD_SPEED 1180
#define LEFT_FORWARD_SPEED 1280
#define FORWARD_TIME_MS 10589
#define TURN_SPEED      900
#define TURN_TIME_MS    2700


void tractor_stop(void)
{
    motors_set_speeds(0, 0);
}

void tractor_forward(int speed)
{
    motors_set_speeds(speed, speed);
}

void tractor_backward(int speed)
{
    motors_set_speeds(-speed, -speed);
}

void tractor_rotate_left(int speed)
{
    motors_set_speeds(-speed, speed);
}

void tractor_rotate_right(int speed)
{
    motors_set_speeds(speed, -speed);
}

void move_one_meter(void)
{
    motors_set_speeds(LEFT_FORWARD_SPEED, RIGHT_FORWARD_SPEED);
    sleep_ms(FORWARD_TIME_MS);
    tractor_stop();
}
void turn_right_90(void)
{
    motors_set_speeds(TURN_SPEED, -TURN_SPEED);
    sleep_ms(TURN_TIME_MS);
    tractor_stop();
}
int main(void) {
    motors_init();
    
    for(int i = 0; i < 4; i++)
    {
        sleep_ms(3000);
        move_one_meter();
        turn_right_90();

    }

    tractor_stop();

    while (true)
    {
        tight_loop_contents();
    }
}