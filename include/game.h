#ifndef GAME_H
#define GAME_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define SCREEN_I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_RESET -1

// Game states
#define GAME_UPDATE 0x75 
#define GAME_PAUSE 0x70
#define GAME_RESET 0x72

// Paddle directions
#define LEFT 0x4C
#define RIGHT 0x52
#define UP 0x55
#define DOWN 0x44

#define PADDLE_OFFSET 2

typedef struct {
  int16_t x, y;
  int16_t width, height;
} shape;

typedef struct {
  int16_t x, y;
} velocity;

void gameInit();
void gameReset();
void gameUpdate();
void gameDraw();
uint8_t gameGetState();

#endif