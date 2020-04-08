#define BLYNK_PRINT Serial

#define BLYNK_USE_DIRECT_CONNECT

#include <M5StickC.h>
#include <BlynkSimpleEsp32_BLE.h>
#include <BLEDevice.h>
#include <BLEServer.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
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
  Wire.beginTransmission(0x38);
  Wire.write(Addr);
  for (int i = 0; i < Length; i++)
  {
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

  Serial.println("Waiting for connections...");
  Blynk.setDeviceName("M5StickC_RoverC");
  Blynk.begin(auth);
  Serial.println("connected");

  Wire.begin(0, 26, 10000);
  SetChargingCurrent(4);
  Serial.println("setup completed");
  digitalWrite(GPIO_NUM_10, LOW);
}

uint8_t l_rightleft;
uint8_t l_updown;
uint8_t r_rightleft;

int8_t speed_sendbuff[4] = {0};
float f, b, l, r, rr, rl = 0.0;
float limit = 1.0;

int FORWARD[4]     = {50, 50, 50, 50};
int LEFT[4]        = {-50, 50, 50, -50};
int BACKWARD[4]    = {-50, -50, -50, -50};
int RIGHT[4]       = {50, -50, -50, 50};
int ROTATE_L[4]    = {-30, 30, -30, 30};
int ROTATE_R[4]    = {30, -30, 30, -30};


// 無入力の時の値
// スティックを倒すと00~7F~FFの値で変化する
const uint8_t LEVER_LEFT_CENTER = 0x7F;

void control() {
  const uint8_t * ptr = (const uint8_t *) data;

  if (l_updown != LEVER_LEFT_CENTER) {
  // 左スティックが倒されていたら
    if (l_updown < LEVER_LEFT_CENTER) {
      // 前進
      // スティックの倒され具合により0.0~1.0に設定
      f = ((LEVER_LEFT_CENTER - l_updown / 128.0);
//    printf("f = %f\n", f);
    }
    if (l_updown > LEVER_LEFT_CENTER) {
      // 後退
      // スティックの倒され具合により0.0~1.0に設定
      b = ((l_updown - LEVER_LEFT_CENTER) / 128.0);
//    printf("b = %f\n", b);
    }
  }
  else {
    // センターの場合リセットする
    f = 0.0;
    b = 0.0;
  }
  if (l_rightleft != LEVER_LEFT_CENTER) {
    if (l_rightleft < LEVER_LEFT_CENTER) {
      // 左進
      l = ((LEVER_LEFT_CENTER - l_rightleft) / 128.0);
//    printf("l = %f\n", l);
    }
    if (l_rightleft > LEVER_LEFT_CENTER) {
      // 右進
      r = ((l_rightleft - LEVER_LEFT_CENTER) / 128.0);
//    printf("r = %f\n", r);
    }   
  }
  else {
    l = 0.0;
    r = 0.0;
  }
  if (r_rightleft != LEVER_LEFT_CENTER) {
    if (r_rightleft < LEVER_LEFT_CENTER) {
      // 左回転
      rl = ((LEVER_LEFT_CENTER - r_rightleft) / 128.0);
//    printf("rl = %f\n", rl);
    }
    if (r_rightleft > LEVER_LEFT_CENTER) {
      // 右回転
      rr = ((r_rightleft - LEVER_LEFT_CENTER) / 128.0);
//    printf("rr = %f\n", rr);
    }
  }
  else {
    rl = 0.0;
    rr = 0.0;
  }

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
  printf("limit = %f\n", limit);
  if (1.0 < limit) {
    limit = 1.0;
  }
  for (int i = 0; i < 4; i++) {
    speed_sendbuff[i] = speed_sendbuff[i] * limit;
  }
  Serial.printf("power = %d, %d, %d, %d\n", speed_sendbuff[0], speed_sendbuff[1], speed_sendbuff[2], speed_sendbuff[3]);

  I2CWritebuff(0x00, (uint8_t *)speed_sendbuff, 4);
}

void loop()
{
  Blynk.run();
  control();
}

BLYNK_WRITE(V1) {
  l_rightleft = param[0].asInt();
}
BLYNK_WRITE(V2) {
  l_updown = param[0].asInt();
}
BLYNK_WRITE(V3) {
  r_rightleft = param[0].asInt();
}
BLYNK_WRITE(V4) {
  int16_t temp = param[0].asInt();
}
