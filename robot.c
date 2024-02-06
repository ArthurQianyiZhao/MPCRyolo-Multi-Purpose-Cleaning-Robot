#include <AFMotor.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <LSM6.h>
#include <Servo.h>

int pos = 0; //���
Servo servo1;   //�����������servo1
Servo servo2;   //�����������servo2

void steeringSetup() {
    servo1.attach(10);    //1�Ŷ�� arm
    servo1.write(110);  //110?

    servo2.attach(9);     //2�Ŷ�� towel
    servo2.write(0);  //-45?
}

void armDown() {
    servo1.write(0);
}

void armUp() {
    servo1.write(110);
}

void towelDown() {
    servo2.write(20);
}

void towelUp() {
    servo1.write(0);
}

/*********IMU***********/
LIS3MDL mag;
LSM6 imu;
LIS3MDL::vector<int16_t> m_min = { -32767, -32767, -32767 };
LIS3MDL::vector<int16_t> m_max = { +32767, +32767, +32767 };

float heading1;
float heading;

void IMUsetup() {
    Wire.begin();

    if (!mag.init())
    {
        Serial.println("Failed to detect and initialize LIS3MDL magnetometer!");
        while (1);
    }
    mag.enableDefault();

    if (!imu.init())
    {
        Serial.println("Failed to detect and initialize LSM6 IMU!");
        while (1);
    }
    imu.enableDefault();
}

template <typename T> float computeHeading(LIS3MDL::vector<T> from)
{
    LIS3MDL::vector<int32_t> temp_m = { mag.m.x, mag.m.y, mag.m.z };

    // copy acceleration readings from LSM6::vector into an LIS3MDL::vector
    LIS3MDL::vector<int16_t> a = { imu.a.x, imu.a.y, imu.a.z };

    // subtract offset (average of min and max) from magnetometer readings
    temp_m.x -= ((int32_t)m_min.x + m_max.x) / 2;
    temp_m.y -= ((int32_t)m_min.y + m_max.y) / 2;
    temp_m.z -= ((int32_t)m_min.z + m_max.z) / 2;

    // compute E and N
    LIS3MDL::vector<float> E;
    LIS3MDL::vector<float> N;
    LIS3MDL::vector_cross(&temp_m, &a, &E);
    LIS3MDL::vector_normalize(&E);
    LIS3MDL::vector_cross(&a, &E, &N);
    LIS3MDL::vector_normalize(&N);

    // compute heading
    float heading = atan2(LIS3MDL::vector_dot(&E, &from), LIS3MDL::vector_dot(&N, &from)) * 180 / PI;
    if (heading < 0) heading += 360;
    return heading;
}


float computeHeading()
{
    return computeHeading((LIS3MDL::vector<int>) { 1, 0, 0 });
}

/******************/

AF_DCMotor motor1(3);
AF_DCMotor motor2(4);
AF_DCMotor motor3(1);
AF_DCMotor motor4(2);

int control = 'x';
int inByte = 'x';

const int TrigPin = 42;  //�м�
const int EchoPin = 43;

const int TrigPin2 = 44; //���
const int EchoPin2 = 45;

const int TrigPin3 = 46; //�Ҳ�
const int EchoPin3 = 47;

float distance;
float distanceL;
float distanceR;

int flag = 0;  //����Ƿ�ס����
int flagL = 0;
int flagR = 0;


int flag_lastTuring = 0; //0��ת1��ת

void motorSpeed(int s1, int s2, int s3, int s4) {
    motor1.setSpeed(s1);
    motor2.setSpeed(s2);
    motor3.setSpeed(s3);
    motor4.setSpeed(s4);
}

void ahead() {
    motorSpeed(200, 200, 200, 200);
    motor1.run(FORWARD);
    motor2.run(FORWARD);
    motor3.run(FORWARD);
    motor4.run(FORWARD);
    delay(150);
}


void retreat() {
    motorSpeed(200, 200, 200, 200);
    motor1.run(BACKWARD);
    motor2.run(BACKWARD);
    motor3.run(BACKWARD);
    motor4.run(BACKWARD);
    delay(150);
}

void left() {
    flag_lastTuring = 0;
    motorSpeed(250, 250, 250, 250);
    motor1.run(BACKWARD);
    motor2.run(BACKWARD);
    motor3.run(FORWARD);
    motor4.run(FORWARD);
    delay(150);

}

void right() {
    flag_lastTuring = 1;
    motorSpeed(250, 250, 250, 250);
    motor1.run(FORWARD);
    motor2.run(FORWARD);
    motor3.run(BACKWARD);
    motor4.run(BACKWARD);
    delay(150);
}

void still() {
    motor1.run(RELEASE);
    motor2.run(RELEASE);
    motor3.run(RELEASE);
    motor4.run(RELEASE);
    delay(30);
}


void ranging(int trigPin_x, int echoPin_x, float* distance_p, int danger_len, int* flag_p) {
    digitalWrite(trigPin_x, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin_x, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin_x, LOW);

    // ��������ȣ������������
    *distance_p = pulseIn(echoPin_x, HIGH) / 58.00;
    if (*distance_p < 12) {
        *flag_p = 1;
    }
    else {
        *flag_p = 0;
    }

}



void ultrasonicSetup() {
    Serial.begin(9600);
    pinMode(TrigPin, OUTPUT);
    pinMode(EchoPin, INPUT);
    pinMode(TrigPin2, OUTPUT);
    pinMode(EchoPin2, INPUT);
    pinMode(TrigPin3, OUTPUT);
    pinMode(EchoPin3, INPUT);
    Serial.println("Ultrasonic sensor:");
}

void ultrasonicLoop() {

    ranging(TrigPin, EchoPin, &distance, 5, &flag);

    ranging(TrigPin2, EchoPin2, &distanceL, 30, &flagL);

    ranging(TrigPin3, EchoPin3, &distanceR, 30, &flagR);



}



/**********״̬������***********/

char type;
//int flag;  //�������Ǳ��Ѿ�����//�Ƿ�ס����
//int flagForP = 0;  //�ж�Ӧ�ü�ס���Ƿ���

int counterS = 0;
int counter = 0;  //��¼ѭ������
int state = 0;  //��¼��ǰ���ڵ�״̬���



void avoid() {  //state 0
  //�����߼�����
    if (
        (!flagL) && (!flagR)
        ) {

        ahead();


    }
    else if (
        (flagL) && (!flagR)
        ) {

        right();
        still();
    }
    else if (
        (flagL) && (flagR)
        ) {

        flag_lastTuring ? right() : left();  //0��ת1��ת
        still();
    }
    else {

        left();
        still();
    }

    if (counterS < 40) {
        counterS += 1;
    }
    else {
        counterS = 0;
        state = 1;
    }


}

void searchAround() {  //state 1
  //����ǰcounter��0
    if (counter < 4) {
        right();
        Serial.println("right");
    }
    else if (counter < 8) {
        still();
    }
    else {
        counter = -1;//׼����תһ���Ƕ�
        Serial.println("");

    }

    counter += 1;

    if (counterS < 40) {
        counterS += 1;
    }
    else {
        counterS = 0;
        counter = 0;
        state = 0;//�ص�state 0
    }
}

//��û��ȫд�����
void toGarbage() {  //state 2  ������ǰ��
    switch (inByte) {
    case 'w':
        ahead();
        ahead();
        ahead();
        Serial.println(control);
        break;
    case 's':
        retreat();
        Serial.println(control);
        break;
    case 'a':
        left();
        Serial.println(control);
        break;
    case 'd':
        right();
        Serial.println(control);
        break;
    default:
        still();
        Serial.println(control);

    }

    //����Ƿ�ס��Ŀ��  ���ﻹûд
    if (flag) {  //��ס����

      //���ϼ���  
        armDown();
        Serial.write('f'); //֪ͨ��ݮ���ҵ�����

        //ת���ʼ����ķ�����
        state = 5;
    }

}



void toQR() {  //state 3 �ҵ���ά�룬����ǰ��
    switch (inByte) {
    case 'w':
        ahead();
        ahead();
        ahead();
        Serial.println(control);
        break;
    case 's':
        retreat();
        Serial.println(control);
        break;
    case 'a':
        left();
        Serial.println(control);
        break;
    case 'd':
        right();
        Serial.println(control);
        break;
    default:
        still();
        Serial.println(control);

        //����Ƿ�ס��Ŀ��  ���ﻹûд
        if (
            flagL && flagR  //��סQR
            ) {

            //��������  
            armUp();
            Serial.write('p'); //֪ͨ��ݮ���ҵ�QR

            retreat();
            retreat();
            retreat();
            retreat();

            //ת���ʼ����
            state = 4;
        }

    }
}

void turnTo() {  //state 4 ת���ʼ�ĳ���
    int min = ((int)heading1 - 20 + 360) % 360;
    int max = ((int)heading1 + 20 + 360) % 360;
    if (min < max) {
        if (heading < min) {
            right();
        }
        else if (heading > max) {
            left();
        }
        else {
            //�ﵽԤ�ڣ��˳�
            still();
            state = 0;
        }
    }
    else {  //Ԥ�ڵ���������м��Խ0��
        if (heading<max || heading>min) {
            //�ﵽԤ�ڣ��˳�
            still();
            state = 0;
        }
        else {
            right();
        }
    }
}

void turnOp() {  //state 5 ת���ʼ����ķ���
    int min = ((int)heading1 - 180 - 20 + 360) % 360;
    int max = ((int)heading1 - 180 + 20 + 360) % 360;
    if (min < max) {
        if (heading < min) {
            right();
        }
        else if (heading > max) {
            left();
        }
        else {
            //�ﵽԤ�ڣ��˳�
            still();
            state = 0;
        }
    }
    else {  //Ԥ�ڵ���������м��Խ0��
        if (heading<max || heading>min) {
            //�ﵽԤ�ڣ��˳�
            still();
            state = 0;
        }
        else {
            right();
        }
    }
}


void toCoin() {  //state 6

}

void toWater() {  //state 7

}


/**********����ݮ��ͨѶ����***************/
//inByte = 'x';
//control = 'x';

void listenFromPi() {
    // read from port 3, send to port 0:
    if (Serial3.available()) {
        inByte = Serial3.read();

        //�������ಿ��
        if (
            (char)inByte == '1' //ֽ��
            ) {
            still();
            state = 2;//�л��������ռ�״̬

            type = '1';
            Serial.write('1');
        }
        else if (
            (char)inByte == '2' //����
            ) {
            still();
            state = 2;
            Serial.println(inByte);
            type = '2';
            Serial.write('2');
        }
        else if (
            (char)inByte == '3' //������
            ) {
            still();
            state = 2;

            type = '3';
            Serial.write('3');
        }

        //QR���ಿ��
        if (
            type == '1' && (char)inByte == 'v'
            ) {
            still();
            state = 3;

            Serial.write('v');
        }
        else if (
            type == '2' && (char)inByte == 'n'
            ) {
            still();
            state = 3;

            Serial.write('n');
        }
        else if (
            type == '3' && (char)inByte == 'm'
            ) {
            still();
            state = 3;

            Serial.write('m');
        }

        //�����������
        if (
            (char)inByte == 'w' ||
            (char)inByte == 's' ||
            (char)inByte == 'a' ||
            (char)inByte == 'd' ||
            (char)inByte == 'x'
            ) {
            control = (char)inByte;
        }


    }
    // read from port 0, send to port 3:
    if (Serial.available()) {
        inByte = Serial3.read();
        Serial.println(inByte);
        //�������ಿ��
        if (
            (char)inByte == '1' //ֽ��
            ) {
            still();
            state = 2;//�л��������ռ�״̬

            type = '1';
            Serial.write('1');
        }
        else if (
            (char)inByte == '2' //����
            ) {
            still();
            state = 2;

            type = '2';
            Serial.write('2');
        }
        else if (
            (char)inByte == '3' //������
            ) {
            still();
            state = 2;

            type = '3';
            Serial.write('3');
        }

        //QR���ಿ��
        if (
            type == '1' && (char)inByte == 'v'
            ) {
            still();
            state = 3;

            Serial.write('v');
        }
        else if (
            type == '2' && (char)inByte == 'n'
            ) {
            still();
            state = 3;

            Serial.write('n');
        }
        else if (
            type == '3' && (char)inByte == 'm'
            ) {
            still();
            state = 3;

            Serial.write('m');
        }

        //�����������
        if (
            (char)inByte == 'w' ||
            (char)inByte == 's' ||
            (char)inByte == 'a' ||
            (char)inByte == 'd' ||
            (char)inByte == 'x'
            ) {
            control = (char)inByte;
        }
    }
}

/******************************/

void setup() {
    Serial.begin(9600);
    Serial3.begin(9600);
    steeringSetup();
    IMUsetup();
    ultrasonicSetup();
    mag.read();
    imu.read();
    heading1 = computeHeading();
}

void loop() {
    ultrasonicLoop();


    mag.read();
    imu.read();
    heading = computeHeading();
    Serial.println(state);

    listenFromPi();

    switch (state) { //״̬��
    case 0:
        avoid();
        break;
    case 1:
        searchAround();
        break;
    case 2:
        toGarbage();
        break;
    case 3:
        toQR();
        break;
    case 4:
        turnTo();
        break;
    case 5:
        turnOp();
        break;


    }


}
