#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <StringUtils.h>
#include <WString.h>
#include <OneButton.h>

// WIFI账号密码
const char ssid[] = "ciaos";
const char pswd[] = "Arimura Hinae";

#define minimum(a, b) (((a) < (b)) ? (a) : (b))

// main程序需要用的变量
int currentSec,
    currentMinute,
    currentHour,
    weekDay,
    currentWeek = 10,
    page_num = 0,
    page_num_read = 0;

float temp_read = 50.2,
      humi_read = 50.2,
      pres_read = 50.2,
      alti_read = 50.2,
      pm_read = 500.02,
      tvoc_read = 50.02;

String wind_speed = "";

unsigned long myTime = 0;

char tempc,
    humic,
    presc,
    altic,
    pm25;

#include "O128.h"
#include "humi.h"
#include "temp.h"
#include "chars.h"
#include "network.h"

WiFiClient wc;
PubSubClient pc(wc);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clk = TFT_eSprite(&tft);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000);
OneButton btn = OneButton(D2, true, true);

static void singleClick();
void newpage1();
void newpage2();
void page3();
void connectWifi(const char *wifiName, const char *wifiPassword, uint8_t waitTime);
void getMQTT(char *topic, byte *payload, unsigned int length);
uint8_t connectMQTT();
void renderJPEG(int xpos, int ypos);
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos);
void setweekday(String weekday, int num);
void page_not_connect();
void display_time();

void setup()
{
  Serial.begin(115200);

  tft.begin();
  Serial.println();
  connectWifi(ssid, pswd, 10);
  connectMQTT();
  Serial.printf("macAddress is %s", WiFi.macAddress().c_str());

  tft.setRotation(0);
  tft.fillScreen(TFT_WHITE);
  drawArrayJpeg(O128, sizeof(O128), 0, 16); // Draw a jpeg image stored in memory at x,y
  delay(2000);

  btn.attachClick(singleClick);

  page_num = 1;
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED) // 如果WIFI断开,那么尝试重新连接
  {
    Serial.println("WIFI is break,try to connect...");
    connectWifi(ssid, pswd, 10);
    page_not_connect();
    connectMQTT();
  }
  else
  {
    if (pc.connected())
    {            // 如果还和MQTT服务器保持连接
      pc.loop(); // 发送心跳信息
    }
    else
    {
      connectMQTT(); // 如果和MQTT服务器断开连接,那么重连
    }

    btn.tick();

    delay(100);

    if (page_num == 1)
    {
      newpage1();
    }
    if (page_num == 2)
    {
      newpage2();
    }
  }
}

void newpage2()
{
  clk.setColorDepth(16);         // 位深度8
  clk.createSprite(128, 160);    // 创建窗口
  Serial.println(clk.created()); // 若创建成功则返回真
  clk.fillSprite(TFT_WHITE);

  display_time();
  clk.drawRect(0, 0, 128, 160, tft.alphaBlend(128, TFT_RED, TFT_WHITE));
  clk.drawFastHLine(0, 29, 128, tft.alphaBlend(128, TFT_RED, TFT_WHITE));
  clk.drawFastHLine(0, 95, 128, tft.alphaBlend(128, TFT_RED, TFT_WHITE));
  clk.drawFastVLine(64, 30, 128, tft.alphaBlend(128, TFT_RED, TFT_WHITE));

  clk.loadFont(msyh16);

  clk.setTextColor(TFT_BLACK);
  clk.setCursor(14, 35);
  clk.println("气压");
  clk.setCursor(16, 50);
  clk.println("kPa");
  clk.setCursor(2, 66);
  clk.printf("%.3f", pres_read);

  clk.setCursor(14 + 64 + 2, 35);
  clk.println("海拔");
  clk.setCursor(16 + 64 + 8, 50);
  clk.println("m");
  clk.setCursor(2 + 64 + 10, 66);
  clk.println(alti_read);

  clk.setCursor(16, 35 + 65);
  clk.println("CO2");
  clk.setCursor(16, 50 + 65);
  clk.println("ppm");
  clk.setCursor(10, 66 + 65 + 2);
  clk.println(pm_read);

  clk.setCursor(14 + 64 + 2, 35 + 65);
  clk.println("甲烷");
  clk.setCursor(16 + 64, 50 + 65);
  clk.println("ppm");
  clk.setCursor(2 + 64 + 10 + 2, 66 + 65 + 2);
  clk.println(tvoc_read);

  page_num = 2;
  clk.pushSprite(0, 0);
  clk.deleteSprite();
  clk.unloadFont();
}

void page3()
{
  if (page_num != 3)
    tft.fillScreen(TFT_BLACK);
  page_num = 3;

  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Draw some random filled ellipses
  for (int i = 0; i < 20; i++)
  {
    int rx = random(40);
    int ry = random(40);
    int x = rx + random(160 - rx - rx);
    int y = ry + random(128 - ry - ry);
    tft.fillEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
  tft.fillScreen(TFT_BLACK);

  // Draw some random outline ellipses
  for (int i = 0; i < 20; i++)
  {
    int rx = random(40);
    int ry = random(40);
    int x = rx + random(160 - rx - rx);
    int y = ry + random(128 - ry - ry);
    tft.drawEllipse(x, y, rx, ry, random(0xFFFF));
  }

  delay(2000);
}

void connectWifi(const char *wifiName, const char *wifiPassword, uint8_t waitTime)
{
  WiFi.mode(WIFI_STA);                // 设置无线终端模式
  WiFi.disconnect();                  // 清除配置缓存
  WiFi.begin(wifiName, wifiPassword); // 开始连接
  uint8_t count = 0;
  while (WiFi.status() != WL_CONNECTED)
  { // 没有连接成功之前等待
    delay(1000);
    Serial.printf("connect WIFI...%ds\r\n", ++count);
    if (count >= waitTime)
    { // 超过设定的等待时候后退出
      Serial.println("connect WIFI fail");
      return;
    }
  }
  // 连接成功,输出打印连接的WIFI名称以及本地IP
  Serial.printf("connect WIFI %s success,local IP is %s\r\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void getMQTT(char *topic, byte *payload, unsigned int length)
{
  Serial.printf("get data from %s\r\n", topic); // 输出调试信息,得知是哪个主题发来的消息
  String n = "";
  for (unsigned int i = 0; i < length; ++i)
  {                                 // 读出信息里的每个字节
    Serial.print((char)payload[i]); // 以文本形式读取就这样,以16进制读取的话就把(char)删掉
    n.concat((char)payload[i]);
  }
  int i = 0;
  for (TextParser p(n, ','); p.parse(); i++)
  {
    Serial.println(p);
    switch (i)
    {
    case 0:
      temp_read = p.toFloat();
      break;
    case 1:
      humi_read = p.toFloat();
      break;
    case 2:
      pres_read = p.toFloat();
      pres_read = pres_read / 1000;
      break;
    case 3:
      alti_read = p.toFloat();
      break;
    case 4:
      pm_read = p.toFloat();
      break;
    case 5:
      tvoc_read = p.toFloat();
    default:
      break;
    }
  }
  Serial.println();
}

uint8_t connectMQTT()
{
  if (WiFi.status() != WL_CONNECTED)
    return -1;                          // 如果网没连上,那么直接返回
  pc.setServer("broker.emqx.io", 1883); // 设置MQTT服务器IP地址以及端口(一般固定是1883)
  if (!pc.connect("tft"))
  {
    Serial.println("connect MQTT fail");
    return -1;
  }
  String topic = "tempsss";
  pc.subscribe(topic.c_str());
  pc.setCallback(getMQTT); // 绑定订阅回调函数
  Serial.println("connect MQTT success");
  return 0;
}

void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos)
{

  int x = xpos;
  int y = ypos;

  JpegDec.decodeArray(arrayname, array_size);

  // jpegInfo(); // Print information from the JPEG file (could comment this line out)

  renderJPEG(x, y);

  Serial.println("#########################");
}

void renderJPEG(int xpos, int ypos)
{

  // retrieve information about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.readSwappedBytes())
  {

    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos; // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x)
      win_w = mcu_w;
    else
      win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y)
      win_h = mcu_h;
    else
      win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // draw image MCU block only if it will fit on the screen
    if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
    {
      tft.pushRect(mcu_x, mcu_y, win_w, win_h, pImg);
    }
    else if ((mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;

  // print the results to the serial port
  Serial.print(F("Total render time was    : "));
  Serial.print(drawTime);
  Serial.println(F(" ms"));
  Serial.println(F(""));
}

void setweekday(String weekday, int num)
{
  // if (num != currentWeek)
  // {
  //   tft.fillRect(46, TEMPFIN_HEIGHT + HUMIFIN_HEIGHT + 2, 128, 16, TFT_BLACK);
  // }
  clk.setCursor(104, 12, 1);
  clk.setTextColor(TFT_DARKGREEN);
  clk.setTextSize(1);
  clk.println(weekday);
  // currentWeek = num;
}

void jpegInfo()
{
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F("Width      :"));
  Serial.println(JpegDec.width);
  Serial.print(F("Height     :"));
  Serial.println(JpegDec.height);
  Serial.print(F("Components :"));
  Serial.println(JpegDec.comps);
  Serial.print(F("MCU / row  :"));
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F("MCU / col  :"));
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F("Scan type  :"));
  Serial.println(JpegDec.scanType);
  Serial.print(F("MCU width  :"));
  Serial.println(JpegDec.MCUWidth);
  Serial.print(F("MCU height :"));
  Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}

void page_not_connect()
{
  // 做一个没有连接网络时的页面
  clk.setColorDepth(16);         // 位深度a
  clk.createSprite(128, 160);    // 创建窗口
  Serial.println(clk.created()); // 若创建成功则返回真
  clk.fillScreen(TFT_WHITE);
  clk.pushImage(13, 13, NETWORK_WIDTH, NETWORK_HEIGHT, network);
  clk.setCursor(13, 80);
  clk.setTextColor(TFT_BLACK);
  clk.loadFont(msyh16); // 设定我们制作的字体
  clk.println("未连接到网络");
  clk.pushSprite(0, 0);
  clk.deleteSprite();
  clk.unloadFont(); // 释放字库,节省RAM
}

void newpage1()
{
  clk.setColorDepth(16);         // 位深度8
  clk.createSprite(128, 160);    // 创建窗口
  Serial.println(clk.created()); // 若创建成功则返回真
  clk.fillSprite(TFT_WHITE);

  display_time();

  clk.loadFont(msyh16);

  clk.pushImage(0, 45, TEMPFIN_WIDTH, TEMPFIN_HEIGHT, tempfin);
  clk.setCursor(64, 40 + int(TEMPFIN_HEIGHT / 2));
  clk.setTextColor(TFT_BLACK);
  clk.setTextSize(1);
  clk.print(temp_read);
  clk.print("°C");

  clk.pushImage(0, TEMPFIN_HEIGHT + 50, HUMIFIN_WIDTH, HUMIFIN_HEIGHT, humifin);
  clk.setCursor(64, int(HUMIFIN_HEIGHT / 2 + TEMPFIN_HEIGHT) + 45);
  clk.setTextColor(TFT_BLACK);
  clk.setTextSize(1);
  clk.print(humi_read);
  clk.println("%");

  clk.drawRect(0, 0, 128, 160, tft.alphaBlend(128, TFT_RED, TFT_WHITE));
  clk.drawFastHLine(0, 29, 128, tft.alphaBlend(128, TFT_RED, TFT_WHITE));

  page_num = 1;
  clk.pushSprite(0, 0);
  clk.deleteSprite();
  clk.unloadFont();
}

void display_time()
{
  timeClient.update();

  currentSec = timeClient.getSeconds();
  currentMinute = timeClient.getMinutes();
  currentHour = timeClient.getHours();
  weekDay = timeClient.getDay();

  // 显示时间    size为2的情况下一个字符12px
  clk.setTextDatum(CC_DATUM); // 设置文本数据
  clk.setTextColor(TFT_BLACK);
  clk.setTextSize(2);
  clk.setCursor(4, 8, 1);
  // clk.setSwapBytes(true);
  Serial.println(timeClient.getFormattedTime());
  clk.println(timeClient.getFormattedTime());

  switch (weekDay)
  {
  case 0:
    setweekday("SUN", weekDay);
    break;
  case 1:
    setweekday("MON", weekDay);
    break;
  case 2:
    setweekday("TUE", weekDay);
    break;
  case 3:
    setweekday("WEN", weekDay);
    break;
  case 4:
    setweekday("THI", weekDay);
    break;
  case 5:
    setweekday("FRI", weekDay);
    break;
  case 6:
    setweekday("SAT", weekDay);
    break;
  default:
    break;
  }
}

static void singleClick()
{
  Serial.println("按键单击");
  if (page_num == 1)
  {
    page_num = 2;
  }
  else
  {
    if (page_num == 2)
    {
      page_num = 1;
    }
  }
}
