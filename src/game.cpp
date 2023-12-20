#include <game.h>

IPAddress PROGMEM localIP(192, 168, 1, 32);
IPAddress PROGMEM gateway(192, 168, 1, 1);
IPAddress PROGMEM subnet(255, 255, 255, 0);
IPAddress PROGMEM primaryDNS(9, 9, 9, 9);
IPAddress PROGMEM secondaryDNS(1, 1, 1, 1);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char indexHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Pong</title>
</head>
<body>
  <button id="btn-update">Update</button>
  <button id="btn-pause">Pause</button>
  <button id="btn-reset">Reset</button>

  <script>
    const GAME_UPDATE = new Uint8Array([0x75]);
    const GAME_PAUSE = new Uint8Array([0x70]);
    const GAME_RESET = new Uint8Array([0x72]);
    const L_UP = new Uint8Array([0x4C, 0x55]);
    const L_DOWN = new Uint8Array([0x4C, 0x44]);
    const R_UP = new Uint8Array([0x52, 0x55]);
    const R_DOWN = new Uint8Array([0x52, 0x44]);

    const ws = new WebSocket(`ws://${window.location.host}/ws`);

    ws.addEventListener("open", (e) => {
      console.log(e)

      document.addEventListener("keydown", (e) => {
        switch (e.keyCode) {
          case 87: // W
            ws.send(L_UP.buffer)
            break;
          case 83: // S
            ws.send(L_DOWN.buffer)
            break;
          case 38: // Up Arrow
            ws.send(R_UP.buffer)
            break;
          case 40: // Down Arrow
            ws.send(R_DOWN.buffer)
            break;
        }
      })

      document
        .querySelector("#btn-update")
        .addEventListener("click", (e) => ws.send(GAME_UPDATE))
      document
        .querySelector("#btn-pause")
        .addEventListener("click", (e) => ws.send(GAME_PAUSE))
      document
        .querySelector("#btn-reset")
        .addEventListener("click", (e) => ws.send(GAME_RESET))
    });
  </script>
</body>
</html>
)rawliteral";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, SCREEN_RESET);
GFXcanvas1 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);

uint8_t gameState;

// Ball
shape ball;
velocity ballVelocity;
const velocity ballVelocityList[] PROGMEM = {
  { .x = 3, .y = 1 },
  { .x = 3, .y = 2 },
  { .x = 3, .y = -1 },
  { .x = 3, .y = -2 },
  { .x = -3, .y = 1 },
  { .x = -3, .y = 2 },
  { .x = -3, .y = -1 },
  { .x = -3, .y = -2 }
};

// Paddles
shape leftPaddle;
shape rightPaddle;
velocity paddleVelocity;
uint8_t leftScore;
uint8_t rightScore;


void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", indexHTML);
}

int16_t clamp(int16_t value, int16_t min, int16_t max) {
  if (value <= min)
    return min;
  if (value >= max)
    return max;
  return value;
}

void handleMsg(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len) {
    switch (data[0]) {
      case GAME_UPDATE:
        gameState = GAME_UPDATE;
        break;
      case GAME_PAUSE:
        gameState = GAME_PAUSE;
        break;
      case GAME_RESET:
        gameState = GAME_RESET;
        break;
      case LEFT:
        if (data[1] == UP) {
          leftPaddle.y = clamp(leftPaddle.y - paddleVelocity.y, 0, SCREEN_HEIGHT - leftPaddle.height);
        } else if (data[1] == DOWN) {
          leftPaddle.y = clamp(leftPaddle.y + paddleVelocity.y, 0, SCREEN_HEIGHT - leftPaddle.height);
        }
        break;
      case RIGHT:
        if (data[1] == UP) {
          rightPaddle.y = clamp(rightPaddle.y - paddleVelocity.y, 0, SCREEN_HEIGHT - rightPaddle.height);
        } else if (data[1] == DOWN) {
          rightPaddle.y = clamp(rightPaddle.y + paddleVelocity.y, 0, SCREEN_HEIGHT - rightPaddle.height);
        }
        break;
    }
  }
}

void handleEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
  void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleMsg(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

uint8_t gameGetState() {
  return gameState;
}

void ballReset() {
  ball.x = SCREEN_WIDTH >> 1;
  ball.y = SCREEN_HEIGHT >> 1;
  ballVelocity = ballVelocityList[random(8)];
}

void gameReset() {
  gameState = GAME_PAUSE;

  ball = { .x = SCREEN_WIDTH >> 1, .y = SCREEN_HEIGHT >> 1, .width = 3, .height = 3 };
  ballReset();

  leftPaddle = { .x = PADDLE_OFFSET, .y = 30, .width = 1, .height = 20 };
  rightPaddle = { .x = SCREEN_WIDTH - 1 - PADDLE_OFFSET, .y = 30, .width = 1, .height = 20 };
  paddleVelocity = { .x = 0, .y = 2 };
  leftScore = 0;
  rightScore = 0;
}

void gameInit() {
  Serial.begin(115200);

  randomSeed(analogRead(0));

  // Connect to WiFi
  if (!WiFi.config(localIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Server stuff
  server.on("/", HTTP_GET, handleRoot);

  ws.onEvent(handleEvent);
  server.addHandler(&ws);

  server.begin();

  // Connect to display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDR)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();

  gameReset();
}

bool collide(shape a, shape b) {
  return (a.x >= b.x - a.width
    && a.x - b.width <= b.x
    && a.y >= b.y - a.height
    && a.y - b.height <= b.y - a.height);
}

void gameUpdate() {
  ball.x += ballVelocity.x;
  ball.y += ballVelocity.y;
  
  // Ball off screen
  if (ball.x <= 0) {
    rightScore++;
    ballReset();
  } else if (ball.x >= SCREEN_WIDTH - ball.width) {
    leftScore++;
    ballReset();
  }

  // Ball Wall Bounce
  if (ball.y <= 0 || ball.y >= (SCREEN_HEIGHT - ball.height)) {
    ballVelocity.y *= -1;
  }

  // Ball Paddle Bounce
  if (collide(ball, leftPaddle)) {
    ballVelocity.x *= -1;
    ball.x += PADDLE_OFFSET;
  } else if (collide(ball, rightPaddle)) {
    ballVelocity.x *= -1;
    ball.x -= PADDLE_OFFSET;
  }
}

void gameDraw() {
  canvas.fillScreen(BLACK);

  // Print Score
  canvas.setTextSize(1);
  canvas.setCursor(25, 2);
  canvas.print(leftScore);
  canvas.setCursor(SCREEN_WIDTH - 35, 2);
  canvas.print(rightScore);

  // Draw Ball
  canvas.fillRect(ball.x, ball.y, ball.width, ball.height, WHITE);

  // Draw Paddles
  canvas.drawFastVLine(leftPaddle.x, leftPaddle.y, leftPaddle.height, WHITE);
  canvas.drawFastVLine(rightPaddle.x, rightPaddle.y, rightPaddle.height, WHITE);

  display.drawBitmap(0, 0, canvas.getBuffer(),
     canvas.width(), canvas.height(), WHITE, BLACK);
  display.display();
}
