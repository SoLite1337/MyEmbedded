#include <Arduino.h>
#include <stdint.h>

#define MOTOR_PIN 17   // Пин управления мотором через ШИМ
#define ENC_CLK   35   // Пин A энкодера (CLK)
#define ENC_DT    36   // Пин B энкодера (DT)
#define ENC_SW    37   // Пин кнопки энкодера (SW)
///////////////////////////////////////////////////////////
#define PWM_CH    0    // Канал LEDC (0..15)
#define PWM_FREQ  5000 // 5 кГц
#define PWM_RES   8    // 8 бит duty 0..255

enum OperatingMode
{
  MODE_SERIAL = 1,  // Режим 1: управление из терминала
  MODE_ENCODER = 2  // Режим 2: управление энкодером
};

OperatingMode currentMode = MODE_SERIAL;

volatile int currentSpeed = 0; 
int lastAppliedSpeed = -1;     // Для отслеживания изменений ШИМ

// Защита от дребезга
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20; 





//Основная функция прерывания для энкодера
void IRAM_ATTR handleEncoder() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  
  // Защита от дребезга контактов (игнорируме импульсы чаще чем 2 мс)
  if (interruptTime - lastInterruptTime > 2) 
  {

    // Если DT != CLK, то при изменении CLK — вращение вправо
    if (digitalRead(ENC_DT) != digitalRead(ENC_CLK)) 
    {
      currentSpeed += 5; // Инкермент PWM от энкодера
      if (currentSpeed > 255) 
      {
        currentSpeed = 255;
      }
    } 
    else 
    {
      currentSpeed -= 5; // Декримент PWM от энкодера
      if (currentSpeed < 0) 
      {
        currentSpeed = 0;
      }
    }

    lastInterruptTime = interruptTime;
  }
}





void io_init()
{
  // Настройка ШИМ
  ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(MOTOR_PIN, PWM_CH);
  ledcWrite(PWM_CH, 0);

  // Настройка пинов энкодера и его кнопки
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  
  // Аппаратное прерывание на пин CLK (по переднему фронту)
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), handleEncoder, RISING);
}



void setup()
{
  Serial.begin(115200);
  delay(2500); 
  Serial.println("System initialized.");
  io_init();
  Serial.println("Ckick SW to change mode.");
  Serial.println("----------------------------------------");
  Serial.println("Current Mode 1: Terminal (comansd: off / low / mid / high)\n");
}

void loop()
{
  //Обработка нажатия кнопки (переключение режимов)
  int reading = digitalRead(ENC_SW);

  if (reading != lastButtonState) 
  {
    lastDebounceTime = millis(); 
  }

  if ((millis() - lastDebounceTime) > debounceDelay) 
  {

    static bool buttonWasPressed = false;
    if (reading == LOW && !buttonWasPressed) 
    {
      buttonWasPressed = true;
      if (currentMode == MODE_SERIAL) 
      {
        currentMode = MODE_ENCODER;
        Serial.println("\n Switched to mode 2 (ENCODER)");
        Serial.print("Current speed: ");
        Serial.println(currentSpeed);
      } 
      else 
      {
        currentMode = MODE_SERIAL;
        Serial.println("\n Switched to mode 1 (Teminal)");
        Serial.println("Enter command (off / low / mid / high):");
      }
    } 

    else if (reading == HIGH) 
    {
      buttonWasPressed = false;
    }
  }
  lastButtonState = reading;


  // Логика режимов
  if (currentMode == MODE_SERIAL) 
  {
    // РЕЖИМ 1
    if (Serial.available()) 
    {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      
      if (cmd == "off") 
      {
        currentSpeed = 0;
        Serial.println("Motor off");
      }
      else if (cmd == "low") 
      {
        currentSpeed = 60;
        Serial.println("Motor low speed 60 PWM");
      }
      else if (cmd == "mid") 
      {
        currentSpeed = 150;
        Serial.println("Motor medium speed 150 PWM");
      }
      else if (cmd == "high") 
      {
        currentSpeed = 255;
        Serial.println("Motor high speed 255 PWM");
      }
      else 
      {
        Serial.println("Unvalid command. Use: off / low / mid / high");
      }
    }
  } 

  else if (currentMode == MODE_ENCODER) 
  {
    // Режим 2 по прерыванию
  }

  //Контроль ШИМ от энкодера
  if (currentSpeed != lastAppliedSpeed) 
  {
    ledcWrite(PWM_CH, currentSpeed);
    lastAppliedSpeed = currentSpeed;

    if (currentMode == MODE_ENCODER) 
    {
      Serial.print("Encoder mode. Speed (PWM): ");
      Serial.println(currentSpeed);
    }
  }
}