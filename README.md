# ESP8266_sensor_station

I decided to build a small air quality sensor station. All the sensors are handled by ESP8266, which sends data to my local Blynk server. The board supports: PMS7003, MH-Z19B, CCS811, SGP30, MICS5524, BME280. All those sensors combined add up to about 70€, which is not the cheapest. Although not all of them are necessary, since CCS811 and SGP30 have very similar characteristics and they are still not as good as MH-Z19B, which is also the most expensive here.

Most of the female headers on PCB have their legs bent, so that they can be soldered like SMD. I did this, since I wanted to reduce the number of THT components, which would interfere with PMS7003 beneath the board. There are two headers that still have THT holes – programming and I2C display output. Those two were soldered with cut pins, so that they wouldn’t reach beneath the PCB. There is also a mistake done on PCB – a connection between GND and “wake” pin of CCS811 module is missing. This was easily fixed by scraping mask off PCB and making a solder jumper to the pin.

My current enclosure doesn’t support the use of I2C display, since I am sending all the data to my Blynk server, but I might design one in the future. Note that everything press-fits into the enclosure, so no screws are used.
