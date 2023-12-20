#include <game.h>

void setup() {
  gameInit();
}

void loop() {
  switch (gameGetState()) {
    case GAME_UPDATE:
      gameUpdate();
      break;
    case GAME_PAUSE:
      break;
    case GAME_RESET:
      gameReset();
      break;
  }

  gameDraw();

  delay(32);
}