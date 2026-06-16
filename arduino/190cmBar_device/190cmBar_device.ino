#include <AccelStepper.h>
#include <Servo.h>
#include <avr/wdt.h>

Servo myServo;  // 创建舵机对象

// 定义机器和绘画范围的参数
const float machineWidth = 1650.0;
const float machineHeight = 1350.0;
const float topleftYX[2] = {350.0, 250.0};
const float pageWidth = machineWidth - topleftYX[1]*2;
const float pageHeight = machineHeight - topleftYX[0];
const float HomePointX = topleftYX[1] + pageWidth / 2;
const float HomePointY = topleftYX[0];

// 定义步进电机驱动器的引脚
#define stepPinLeft 5
#define dirPinLeft 4
#define stepPinRight 6
#define dirPinRight 7

// 步进电机的参数
const float mmPerRotation = 60.0;
const int stepsPerRotation = 400;
const float stepLength = mmPerRotation / stepsPerRotation;

float currentLeftLength, currentRightLength;

AccelStepper stepperLeft(AccelStepper::DRIVER, stepPinLeft, dirPinLeft);
AccelStepper stepperRight(AccelStepper::DRIVER, stepPinRight, dirPinRight);


// 霍尔传感器引脚定义（触发时绘画机复位至page范围的顶部中心，而非page范围的左上角）
const int hallPinLeft = 11;
const int hallPinRight = 12;

// 定义一个全局计数器来跟踪循环次数
unsigned long loopCounter = 0;

// 重启阈值设定
//const unsigned long rebootThreshold = 20; // 设置为需要的重启值
int rebootThreshold = 20;

// 函数声明：执行软件重启
void softwareReboot() {
  // 执行任何必要的清理工作，然后重启
  asm volatile ("  jmp 0");  // 注意：这可能不适用于所有Arduino板
}


float calculateAdjustmentFactor(float x) {
    // 假设在x轴两端，吊线最不准确
    // 这里的minXAdjustment和maxXAdjustment是根据实际情况调整的参数
    float minXAdjustment = -0.06; // 在x轴最左端时的调整值
    float maxXAdjustment = 0.06;  // 在x轴最右端时的调整值

    // 计算线性调整因子
    return minXAdjustment + (maxXAdjustment - minXAdjustment) * (x - topleftYX[1]) / pageWidth;
}

void calculateLengths(float x, float y, float &leftLength, float &rightLength) {
    float adjustmentFactor = calculateAdjustmentFactor(x);

    // 调整后的x坐标
    float adjustedX = x + adjustmentFactor;

    // 使用调整后的x坐标计算吊线长度
    leftLength = sqrt(pow(adjustedX, 2) + pow(y, 2));
    rightLength = sqrt(pow(machineWidth - adjustedX, 2) + pow(y, 2));
}




//计算步进电机需要转动的步数
void calculateSteps(float x, float y, int &leftSteps, int &rightSteps) {
    float targetLeftLength, targetRightLength;
    calculateLengths(x, y, targetLeftLength, targetRightLength);

    // 反转步数的方向
    leftSteps = (currentLeftLength - targetLeftLength) / stepLength;
    rightSteps = (currentRightLength - targetRightLength) / stepLength;


//    Serial.print("Left Steps: ");
//    Serial.print(leftSteps);
//    Serial.print(", Right Steps: ");
//    Serial.println(rightSteps);
}


void moveToPositionSynced(float x, float y) {
    Serial.print("Moving to X: ");
    Serial.print(x);
    Serial.print(", Y: ");
    Serial.println(y);
    delay(50);

    int leftSteps, rightSteps;
    calculateSteps(x, y, leftSteps, rightSteps);

    // 计算左右电机各自的运动距离
    float leftDistance = abs(leftSteps * stepLength);
    float rightDistance = abs(rightSteps * stepLength);

    // 设置左右电机的最大速度，确保两个电机同时到达
    // 速度设置基于各自的运动距离
    float maxSpeed = 400; // 可以根据实际情况调整这个值
    stepperLeft.setMaxSpeed(maxSpeed * (leftDistance / max(leftDistance, rightDistance)));
    stepperRight.setMaxSpeed(maxSpeed * (rightDistance / max(leftDistance, rightDistance)));

    // 开始计时
    unsigned long startTime = millis();
    unsigned long maxDuration = 30000; // 最大运行时间为30秒+++++++++++
    
    // 移动电机
    stepperLeft.moveTo(stepperLeft.currentPosition() - leftSteps);
    stepperRight.moveTo(stepperRight.currentPosition() + rightSteps);

    // 运行电机直到它们都到达目标位置或超时
    while (stepperLeft.distanceToGo() != 0 || stepperRight.distanceToGo() != 0) {
        stepperLeft.run();
        stepperRight.run();
        wdt_reset();

        // 检查是否超时
        if (millis() - startTime > maxDuration) {
            Serial.println("Error: Operation timed out. Rebooting...");
            delay(50);
            softwareReboot();
            delay(2000);
            break; // 超时后跳出循环
        }
    }
    

    // 更新当前长度
    calculateLengths(x, y, currentLeftLength, currentRightLength);
}

void setSyncedSpeeds(float leftLength, float rightLength) {
    float maxSpeed = 400;
    float leftSpeed = maxSpeed * (leftLength / (leftLength + rightLength));
    float rightSpeed = maxSpeed * (rightLength / (leftLength + rightLength));

    stepperLeft.setMaxSpeed(leftSpeed);
    stepperRight.setMaxSpeed(rightSpeed);
}
//复位
void autoHome() {
    Serial.println("in autoHome");


    // 首先，使能步进电机
    digitalWrite(8, LOW);  // Enable the left motor
    digitalWrite(9, LOW);  // Enable the right motor

    // 提高电机复位速度
    stepperLeft.setMaxSpeed(1000);  // 提高电机的最大速度
    stepperRight.setMaxSpeed(1000); // 可以根据实际情况进一步调整
    stepperLeft.setAcceleration(2000);  // 提高加速度
    stepperRight.setAcceleration(2000); // 提高加速度

    // 初始反转步骤
    stepperLeft.move(3000); // 反转左电机一定步数
    Serial.println("autohome Left stepper safty reversed done");
    stepperRight.move(-3000); // 反转右电机一定步数
    Serial.println("autohome Right stepper safty reversed done");
    while (stepperLeft.distanceToGo() != 0 || stepperRight.distanceToGo() != 0) {
        stepperLeft.run();
        stepperRight.run();
    }

    // 复位左电机
    while (digitalRead(hallPinLeft) == LOW) {  // 当霍尔传感器检测到磁场时停止
        stepperLeft.moveTo(stepperLeft.currentPosition() - 5);  // 每次移动更多步数
        stepperLeft.run();
        wdt_reset();
    }

    Serial.println("Left homed");
    // 复位右电机
    while (digitalRead(hallPinRight) == LOW) { // 同上
        stepperRight.moveTo(stepperRight.currentPosition() + 5); // 每次移动更多步数
        stepperRight.run();
        wdt_reset();
    }
    Serial.println("Right homed");

    // 在这里我们不移动到(HomePointX, HomePointY)，而是设置当前位置就是Home位置
    float leftLength, rightLength;
    calculateLengths(HomePointX, HomePointY, leftLength, rightLength);
    int leftSteps = leftLength / stepLength;
    int rightSteps = rightLength / stepLength;

    stepperLeft.setCurrentPosition(leftSteps);
    stepperRight.setCurrentPosition(rightSteps);

    currentLeftLength = leftLength;
    currentRightLength = rightLength;

    // 在这里我们不移动到(HomePointX, HomePointY)，而是设置当前位置就是Home位置
    calculateLengths(HomePointX, HomePointY, currentLeftLength, currentRightLength);

    // 使用全局变量leftSteps和rightSteps
    leftSteps = currentLeftLength / stepLength;
    rightSteps = currentRightLength / stepLength;

    stepperLeft.setCurrentPosition(leftSteps);
    stepperRight.setCurrentPosition(rightSteps);

    Serial.println("Homed to HomePoint");

    // 打印当前坐标和线长
//    Serial.print("Current X: ");
//    Serial.print(HomePointX);
//    Serial.print(", Y: ");
//    Serial.println(HomePointY);
//    Serial.print("Left Line Length: ");
//    Serial.print(currentLeftLength);
//    Serial.print(", Right Line Length: ");
//    Serial.println(currentRightLength);

    Serial.println("Homed to HomePoint");

}

//舵机控制绘画机笔头
void swingServo() {
  myServo.write(19);  // 设置舵机位置到原点
  delay(50);  // 等待50毫秒让舵机移动到位置
//  Serial.println("zeroed");

  myServo.write(54);  // 设置舵机位置到45度
  delay(355);  // 等待300毫秒让舵机摆动到最大角度并保持一段时间++++++++++++++++++++++++
//  Serial.println("swinged");

  myServo.write(19);  // 将舵机返回到原点
  delay(500);  // 等待500毫秒让舵机移动到位置

  Serial.println("Servoed");
}

//移动到随机点位并摆动一次笔头
void moveToRandomPositionAndSwing() {

  // // 在到达指定位置后执行舵机摆动
  // swingServo();
  // 生成画布范围内的随机位置
  Serial.println("moving to random position and swing");
  float randX = random(topleftYX[1], topleftYX[1] + pageWidth);
  float randY = random(topleftYX[0], topleftYX[0] + pageHeight);

//  Serial.print(randX);
//  Serial.print(",");
//  Serial.println(randY);


  // 移动到随机位置
  moveToPositionSynced(randX, randY);

  // 在到达指定位置后执行舵机摆动
  swingServo();
}

void randomlyTriggerFunctions() {

    int j = random (15,30);
    for (int i = 0; i<j; i++){
      delay(3000);
      wdt_reset();//刷新看门狗计时
      Serial.println("waiting");
    }
    moveToRandomPositionAndSwing();

    for (int i = 0; i<j; i++){
      delay(3000);
      wdt_reset();//刷新看门狗计时   
    }
}

//通过arduino serial monitor输入坐标控制绘画机
void moveToInputPosition() {
  if (Serial.available() > 0) {
    // 读取输入的字符串
    String input = Serial.readStringUntil('\n');

    // 找到逗号的位置，这样我们就可以分离X和Y坐标
    int commaIndex = input.indexOf(',');

    // 如果逗号不存在，打印错误信息并返回
    if (commaIndex == -1) {
      Serial.println("Invalid format. Please enter coordinates in the format: X,Y");
      return;
    }

    // 提取X和Y坐标的字符串
    String xString = input.substring(0, commaIndex);
    String yString = input.substring(commaIndex + 1);

    // 将字符串转换为浮点数
    float x = xString.toFloat();
    float y = yString.toFloat();

    // 打印目标位置
//    Serial.print("Moving to X: ");
//    Serial.print(x);
//    Serial.print(", Y: ");
//    Serial.println(y);

    // 移动到目标位置
    moveToPositionSynced(x, y);
  }
}


void setup() {
    
    // 设置步进电机的参数
    stepperLeft.setAcceleration(500);
    stepperRight.setAcceleration(500);

    // 配置步进电机引脚
    pinMode(stepPinLeft, OUTPUT);
    pinMode(dirPinLeft, OUTPUT);
    pinMode(stepPinRight, OUTPUT);
    pinMode(dirPinRight, OUTPUT);
    pinMode(8, OUTPUT);  // EN1 for left motor
    pinMode(9, OUTPUT);  // EN2 for right motor

    digitalWrite(8, LOW);  // Enable the left motor
    digitalWrite(9, LOW);  // Enable the right motor

    pinMode(hallPinLeft, INPUT);
    pinMode(hallPinRight, INPUT);


    myServo.attach(10); // 将舵机附加到pin 10上


    Serial.begin(9600);
    while (!Serial) {
      ; // 等待Serial端口连接
    }
    myServo.write(19); // 舵机初始位置

    autoHome();  // 执行自动复位功能++++++++
    randomSeed(analogRead(0));

    int rebootThreshold = random(10,20);
    Serial.print("machine will reset after");
    Serial.print(rebootThreshold);
    Serial.println("round");
    wdt_enable(WDTO_8S); //设置看门狗

    
}

void loop() {

  // moveToInputPosition();

  randomlyTriggerFunctions();

    // 在每次循环结束时检查是否需要重启
  loopCounter++; // 增加循环计数
  Serial.print(loopCounter);
  Serial.println("round done");
  if (loopCounter >= rebootThreshold) {
        // 在重启前移动到画面中心
    float centerX = machineWidth/2;
    float centerY = machineHeight / 2;
    moveToPositionSynced(centerX, centerY); // 移动到中心
    delay(10000); // 等待移动完成
    softwareReboot();//+++++++++++++++++++++
  }
  wdt_reset();//刷新看门狗计时
}
