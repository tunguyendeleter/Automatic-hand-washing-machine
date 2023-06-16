# AUTOMATIC HAND-WASHING MACHINE
### SYSTEM REQUIREMENTS
#### 1. Functions
-	Collect human body temperature parameters
-	Show warning
-	Engine control
-	Human detection
-	Temperature display
-	Update temperature data through smart device

#### 2. Non-Functions
-	Measuring temperature range: 34 - 36°C with an accuracy of ±0.1°C
-	The measuring range of human detection distance is about :5 - 10 cm
-	Servo rotation angle approx. :180°


### BLOCK DIAGRAM 
!["project"](blockdiagram.jpg)  <br />
**Central processing block**: to collect and process data from sensors then control output devices. <br />
**Input block** <br />
    Temperature sensor: to measures body temperature in a certain temperature range, transmits data to microprocessor <br />
    Ultrasonic sensor:  to measure the distance when hand washing <br />
**Output block** <br />
    LCD  : to display body temperature. <br />
    Servo motor : to open or close faucet washing hands. <br />
    LED : to notify alert tasks to users. <br />
**Power suppy block**: to in charge of delivering power to all other blocks in the remaining system. <br />
**Firebase block** : to get processed data from central processing block and store it on firebase interface <br />


!["blockdiagram"](thread.jpg) <br />
- **Core 0** will be used to control the LCD, receive signals from the temperature sensor MLX90614, and send temperature data to the firebase. 
- **Core 1** will be used to measure distance using the HC-SR04 sensor, control led, and servo.

### PROJECT
