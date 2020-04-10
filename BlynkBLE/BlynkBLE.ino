#define BLYNK_PRINT Serial

#define BLYNK_USE_DIRECT_CONNECT

#include <M5StickC.h>
#include <BlynkSimpleEsp32_BLE.h>
#include <BLEDevice.h>
#include <BLEServer.h>

// UART
HardwareSerial serial_ext(2);

int u_read() {
  int ret = -1;
  int rx_size = 0;
  uint8_t rx_buffer[10];
  if (serial_ext.available()>0) {
    rx_size = serial_ext.readBytes(rx_buffer, 1);
    if (rx_buffer[0] == 0xFF) {
      if (serial_ext.available()>0) {
        rx_size = serial_ext.readBytes(rx_buffer, 1);
        if (rx_buffer[0] == 0xAA) {
          if (serial_ext.available()>0) {
            rx_size = serial_ext.readBytes(rx_buffer, 1);
            ret = rx_buffer[0];
            printf("%d - %d\n", rx_size, ret);
          }
        }
      }
    }
  }
  printf("uread:%d\n", ret);
  return ret;
}

// BlynkのAuth Token
char auth[] = "NNN7TXQARjFvfytycvg3xN_-eELLkIQJ";

void SetChargingCurrent(uint8_t CurrentLevel)
{
  Wire1.beginTransmission(0x34);
  Wire1.write(0x33);
  Wire1.write(0xC0 | (CurrentLevel & 0x0f));
  Wire1.endTransmission();
}

// 指定アドレスにｎバイト書き込み
uint8_t I2CWritebuff(uint8_t Addr, uint8_t *Data, uint16_t Length)
{
  Serial.printf("power = %d, %d, %d, %d\n", Data[0], Data[1], Data[2], Data[3]);
  Wire.beginTransmission(0x38);
  Wire.write(Addr);
  for (int i = 0; i < Length; i++) {
      Wire.write(Data[i]);
  }
  return Wire.endTransmission();
}

void setup()
{
  M5.begin();
  M5.Lcd.println("Blynk");
  M5.Lcd.println("LED BLE TEST");
  
  pinMode(GPIO_NUM_10, OUTPUT);
  digitalWrite(GPIO_NUM_10, HIGH);
  
  // Debug console
  Serial.begin(115900);
  serial_ext.begin(115200, SERIAL_8N1, 32, 33);

  Serial.println("Waiting for connections...");
  Blynk.setDeviceName("M5StickC_RoverC");
  Blynk.begin(auth);
  Serial.println("connected");

  Wire.begin(0, 26, 10000);
  SetChargingCurrent(4);
  Serial.println("setup completed");
}

int FORWARD[4]     = {50, 50, 50, 50};
int LEFT[4]        = {-50, 50, 50, -50};
int BACKWARD[4]    = {-50, -50, -50, -50};
int RIGHT[4]       = {50, -50, -50, 50};
int ROTATE_L[4]    = {-30, 30, -30, 30};
int ROTATE_R[4]    = {30, -30, 30, -30};

// 無入力の時の値
// スティックを倒すと00~80~FFの値で変化する
const uint8_t LEVER_LEFT_CENTER = 0x80;

uint8_t l_rightleft = LEVER_LEFT_CENTER;
uint8_t l_updown = LEVER_LEFT_CENTER;
uint8_t r_rightleft = LEVER_LEFT_CENTER;

void control() {
  int8_t speed_sendbuff[4] = {0};
  float f, b, l, r, rr, rl = 0.0;
  float limit = 1.0;
  Serial.printf("Lrf:%d Lud:%d Rud:%d\n", l_rightleft, l_updown, r_rightleft);
  Serial.printf("f:%f b:%f l:%f r:%f rr:%f rl:%f\n", f, b, l, r, rr, rl);
  // 左スティックが倒されていたら（上下）
  if ((unsigned)l_updown != (unsigned)LEVER_LEFT_CENTER) {
    if ((unsigned)l_updown < (unsigned)LEVER_LEFT_CENTER) {
      // 前進
      // スティックの倒され具合により0.0~1.0に設定
      f = ((LEVER_LEFT_CENTER - l_updown) / 128.0);
      printf("f = %f\n", f);
    }
    if ((unsigned)l_updown > (unsigned)LEVER_LEFT_CENTER) {
      // 後退
      // スティックの倒され具合により0.0~1.0に設定
      b = ((l_updown - LEVER_LEFT_CENTER) / 128.0);
      printf("b = %f\n", b);
    }
  }
  // 左スティックが倒されていたら（左右）
  if ((unsigned)l_rightleft != (unsigned)LEVER_LEFT_CENTER) {
    if ((unsigned)l_rightleft < (unsigned)LEVER_LEFT_CENTER) {
      // 左進
      l = ((LEVER_LEFT_CENTER - l_rightleft) / 128.0);
//    printf("l = %f\n", l);
    }
    if ((unsigned)l_rightleft > (unsigned)LEVER_LEFT_CENTER) {
      // 右進
      r = ((l_rightleft - LEVER_LEFT_CENTER) / 128.0);
//    printf("r = %f\n", r);
    }   
  }
  // 右スティックが倒されていたら（左右）
  if ((unsigned)r_rightleft != (unsigned)LEVER_LEFT_CENTER) {
    if ((unsigned)r_rightleft < (unsigned)LEVER_LEFT_CENTER) {
      // 左回転
      rl = ((LEVER_LEFT_CENTER - r_rightleft) / 128.0);
//    printf("rl = %f\n", rl);
    }
    if ((unsigned)r_rightleft > (unsigned)LEVER_LEFT_CENTER) {
      // 右回転
      rr = ((r_rightleft - LEVER_LEFT_CENTER) / 128.0);
//    printf("rr = %f\n", rr);
    }
  }
  Serial.printf("f:%f b:%f l:%f r:%f rr:%f rl:%f\n", f, b, l, r, rr, rl);

  // 前後左右回転それぞれをスティックの倒れ具合と掛けて足す
  for (int i = 0; i < 4; i++) {
    speed_sendbuff[i] = FORWARD[i] * f;
    speed_sendbuff[i] += BACKWARD[i] * b;
    speed_sendbuff[i] += RIGHT[i] * r;
    speed_sendbuff[i] += LEFT[i] * l;
    speed_sendbuff[i] += ROTATE_L[i] * rl;
    speed_sendbuff[i] += ROTATE_R[i] * rr;
  }

  for (int i = 0; i < 4; i++) {
    // speed_sendbuffは-100~100を取るのでリミッターをかける
    limit = 100.0 / max(
      abs(speed_sendbuff[3]), max(
        abs(speed_sendbuff[2]), max(
          abs(speed_sendbuff[1]), abs(speed_sendbuff[0])
        )
      )
    );
  }
  //printf("limit = %f\n", limit);
  if (1.0 < limit) {
    limit = 1.0;
  }
  for (int i = 0; i < 4; i++) {
    speed_sendbuff[i] = speed_sendbuff[i] * limit;
  }
//  Serial.printf("power = %d, %d, %d, %d\n", speed_sendbuff[0], speed_sendbuff[1], speed_sendbuff[2], speed_sendbuff[3]);

  I2CWritebuff(0x00, (uint8_t *)speed_sendbuff, 4);
}

int16_t face = 0;

byte RR[] = {20,-20,20,-20};
byte RL[] = {-20,20,-20,20};
byte STP[] = {0,0,0,0};
void loop()
{
  Blynk.run();
  control();
  int r = u_read();
  if (r != -1) {          // UARTの入力があった
    if (face != 0){
      if (r == 1) {
        I2CWritebuff(0x00, (uint8_t *)RR, 4);
      } else if (r = 2) {
        I2CWritebuff(0x00, (uint8_t *)RL, 4);
      }
      delay(50);
      I2CWritebuff(0x00, (uint8_t *)STP, 4);
    }
    for (;;) {           // UARTのクリア
      if (serial_ext.available() == 0) {
        break;
      }
      uint8_t buff[1];
      serial_ext.readBytes(buff, 1);
    }
  }
}

BLYNK_WRITE(V0) {
  face = param[0].asInt();
  if (face == 0){
    digitalWrite(GPIO_NUM_10, HIGH);
  } else {
    digitalWrite(GPIO_NUM_10, LOW);
  }
}
BLYNK_WRITE(V1) {
  l_updown = param[0].asInt();
//  Serial.println("V1:");
//  Serial.println(param[0].asInt());
}
BLYNK_WRITE(V2) {
  l_rightleft = param[0].asInt();
//  Serial.println("V2:");
//  Serial.println(param[0].asInt());
}
BLYNK_WRITE(V3) {
  int16_t temp = param[0].asInt();
//  Serial.println("V3:");
//  Serial.println(param[0].asInt());
}
BLYNK_WRITE(V4) {
  r_rightleft = param[0].asInt();
//  Serial.println("V4:");
//  Serial.println(param[0].asInt());
}
