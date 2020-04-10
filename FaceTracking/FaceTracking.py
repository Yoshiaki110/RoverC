import lcd
import image
import time
import uos
import sys
from pmu import axp192
from fpioa_manager import fm, board_info
from machine import UART
fm.register(35, fm.fpioa.UART2_TX, force=True)
fm.register(34, fm.fpioa.UART2_RX, force=True)
uart_Port = UART(UART.UART2, 115200,8,0,0, timeout=1000, read_buf_len= 4096)
def u_send(d):
    data_packet = bytearray([0xFF,0xAA])
    data_packet.append(d % 256)
    uart_Port.write(data_packet)
    print("send " + str(d))


pmu = axp192()
pmu.enablePMICSleepMode(True)

lcd.init()
lcd.rotation(2) #Rotate the lcd 180deg

try:
    img = image.Image("/flash/startup.jpg")
    lcd.display(img)
except:
    lcd.draw_string(lcd.width()//2-100,lcd.height()//2-4, "Error: Cannot find start.jpg", lcd.WHITE, lcd.RED)

from Maix import I2S, GPIO
import audio
from Maix import GPIO
from fpioa_manager import *

fm.register(board_info.SPK_SD, fm.fpioa.GPIO0)
spk_sd=GPIO(GPIO.GPIO0, GPIO.OUT)
spk_sd.value(1) #Enable the SPK output

fm.register(board_info.SPK_DIN,fm.fpioa.I2S0_OUT_D1)
fm.register(board_info.SPK_BCLK,fm.fpioa.I2S0_SCLK)
fm.register(board_info.SPK_LRCLK,fm.fpioa.I2S0_WS)

wav_dev = I2S(I2S.DEVICE_0)

try:
    player = audio.Audio(path = "/flash/ding.wav")
    player.volume(10)
    wav_info = player.play_process(wav_dev)
    wav_dev.channel_config(wav_dev.CHANNEL_1, I2S.TRANSMITTER,resolution = I2S.RESOLUTION_16_BIT, align_mode = I2S.STANDARD_MODE)
    wav_dev.set_sample_rate(wav_info[1])
    while True:
        ret = player.play()
        if ret == None:
            break
        elif ret==0:
            break
    player.finish()
except:
    pass

fm.register(board_info.BUTTON_A, fm.fpioa.GPIO1)
but_a=GPIO(GPIO.GPIO1, GPIO.IN, GPIO.PULL_UP) #PULL_UP is required here!

if but_a.value() == 0: #If dont want to run the demo
    sys.exit()

fm.register(board_info.BUTTON_B, fm.fpioa.GPIO2)
but_b = GPIO(GPIO.GPIO2, GPIO.IN, GPIO.PULL_UP) #PULL_UP is required here!

fm.register(board_info.LED_W, fm.fpioa.GPIO3)
led_w = GPIO(GPIO.GPIO3, GPIO.OUT)
led_w.value(1) #RGBW LEDs are Active Low

fm.register(board_info.LED_R, fm.fpioa.GPIO4)
led_r = GPIO(GPIO.GPIO4, GPIO.OUT)
led_r.value(1) #RGBW LEDs are Active Low

fm.register(board_info.LED_G, fm.fpioa.GPIO5)
led_g = GPIO(GPIO.GPIO5, GPIO.OUT)
led_g.value(1) #RGBW LEDs are Active Low

fm.register(board_info.LED_B, fm.fpioa.GPIO6)
led_b = GPIO(GPIO.GPIO6, GPIO.OUT)
led_b.value(1) #RGBW LEDs are Active Low


time.sleep(0.5) # Delay for few seconds to see the start-up screen :p

import sensor
import KPU as kpu

err_counter = 0

while 1:
    try:
        sensor.reset() #Reset sensor may failed, let's try some times
        break
    except:
        err_counter = err_counter + 1
        if err_counter == 20:
            lcd.draw_string(lcd.width()//2-100,lcd.height()//2-4, "Error: Sensor Init Failed", lcd.WHITE, lcd.RED)
        time.sleep(0.1)
        continue

sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA) #QVGA=320x240
sensor.run(1)

task = kpu.load(0x300000) # Load Model File from Flash
anchor = (1.889, 2.5245, 2.9465, 3.94056, 3.99987, 5.3658, 5.155437, 6.92275, 6.718375, 9.01025)
# Anchor data is for bbox, extracted from the training sets.
kpu.init_yolo2(task, 0.5, 0.3, 5, anchor)

but_stu = 1

try:
    while(True):
        img = sensor.snapshot() # Take an image from sensor
        bbox = kpu.run_yolo2(task, img) # Run the detection routine
        if bbox:
            bbox.sort(reverse=True, key=lambda x: x.rect()[2] * x.rect()[3])
            c = bbox[0].rect()[0] + (bbox[0].rect()[2] / 2)
            print(str(c) + ' ' + str(len(bbox)))
            if c < 120:
                print('>>>')
                u_send(2)
            if c > 160:
                print('<<<')
                u_send(1)
            first = True;
            for i in bbox:
                color = (255,0,0) if first else (255,255,255)
                thick = 5 if first else 1
                first = False
                r = list(i.rect())
                for j in range(thick):
                    img.draw_rectangle(r, color)
                    r[0] = r[0] + 1
                    r[1] = r[1] + 1
        lcd.display(img)

        if but_a.value() == 0 and but_stu == 1:
            if led_w.value() == 1:
                led_w.value(0)
            else:
                led_w.value(1)
            but_stu = 0
        if but_a.value() == 1 and but_stu == 0:
            but_stu = 1

except KeyboardInterrupt:
    a = kpu.deinit(task)
    sys.exit()
