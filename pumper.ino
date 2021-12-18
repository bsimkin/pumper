typedef struct
{
  unsigned int vStages[10]; // pressure
  unsigned int vStagesCount;
  float vStagesTime[10]; // minutes
  unsigned int gistVac;  // pressure delta
  float pTime;          // minutes
  unsigned int cycles;
  unsigned int ledPin;
} ProfileStruct;


bool vacuumVentOpened = false;
bool pressureVentOpened = false;
bool vaccumPumpOn = false;

int systemPressurePin = A0;
int vacuumPumpPin = 3;
int outVentOpenPin = 4;
int outVentClosePin = 5;
int vacuumVentOpenPin = 6;
int vacuumVentClosePin = 7;
int pressureVentOpenPin = 8;
int pressureVentClosePin = 9;

int fin = 13;
int led1Pin = 11;
int led2Pin = 12;
int statusLedPin = 10;

int extraPin = 9;

bool end = false;

ProfileStruct profile;

void OpenVent(int pin, int sec = 20)
{
  Serial.print("[OPEN VENT] PIN: " + String(pin) + " FOR " + String(sec) + " sec \n");
  digitalWrite(pin, LOW);
  float cycles = sec / 2;
  for (int i = 0; i < cycles; i++)
  {
    delay(2000);
  }
  digitalWrite(pin, HIGH);
}

void CloseVent(int pin)
{
  Serial.print("[CLOSE VENT] PIN: " + String(pin) + "\n");
  digitalWrite(pin, LOW);
  for (int i = 0; i < 40; i++)
  {
    delay(500);
  }
  digitalWrite(pin, HIGH);
}

void setup()
{
  Serial.begin(9600);

  pinMode(outVentOpenPin, OUTPUT);
  pinMode(outVentClosePin, OUTPUT);
  pinMode(vacuumVentOpenPin, OUTPUT);
  pinMode(vacuumVentClosePin, OUTPUT);
  pinMode(pressureVentOpenPin, OUTPUT);
  pinMode(pressureVentClosePin, OUTPUT);

  pinMode(statusLedPin, OUTPUT);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(fin, OUTPUT);

  pinMode(vacuumPumpPin, OUTPUT);

  // выключаем все реле
  digitalWrite(outVentOpenPin, HIGH);
  digitalWrite(outVentClosePin, HIGH);
  digitalWrite(vacuumVentOpenPin, HIGH);
  digitalWrite(vacuumVentClosePin, HIGH);
  digitalWrite(pressureVentOpenPin, HIGH);
  digitalWrite(pressureVentClosePin, HIGH);
  digitalWrite(vacuumPumpPin, HIGH);

  digitalWrite(fin, LOW);
}
void loop()
{
  int profileOrder[] = {0, 1, 0, 1};

  ProfileStruct p[10];

  // ПРОФИЛЬ 0
  p[0].vStages[0] = 77; // давление
  p[0].vStages[1] = 70;
  p[0].vStagesTime[0] = 0.2; // время в минутах
  p[0].vStagesTime[1] = 0.3;
  p[0].vStagesCount = 2; // колличество стадий
  
  p[0].gistVac = 2; // разница давлений
  p[0].pTime = 1;  // время удержания давления в минутах
  p[0].cycles = 1;  // количество циклов
  p[0].ledPin = led1Pin;  // pin диода индикатора профиля

  // ПРОФИЛЬ 1
  p[1].vStages[0] = 62; // давление
  p[1].vStagesTime[0] = 1; // время в минутах
  p[1].vStagesCount = 1; // колличество стадий

  p[1].gistVac = 2; // разница давлений
  p[1].pTime = 1;  // время удержания давления в минутах
  p[1].cycles = 1;  // количество циклов
  p[1].ledPin = led2Pin;  // pin диода индикатора профиля

  while (!end)
  {
    digitalWrite(statusLedPin, HIGH);
    for (int i = 0; i < sizeof profileOrder / sizeof(profileOrder[0]); i++)
    {
      profile = p[profileOrder[i]];
      Serial.print("[START] PROFILE : " + String(profileOrder[i]) + "\n");
      digitalWrite(profile.ledPin, HIGH);
      for (int c = 0; c < profile.cycles; c++)
      {
        OpenVent(outVentOpenPin);
        CloseVent(outVentClosePin);
        CloseVent(vacuumVentClosePin);
        CloseVent(pressureVentClosePin);

        bool vReady = false;

        // 1. Держим давление каждой стадии
        while (!vReady)
        {
          if (!vacuumVentOpened)
          {

            Serial.print("[VACCUM] START OPEN vaccum vent \n");
            OpenVent(vacuumVentOpenPin);
            vacuumVentOpened = true;
          }

          if (!vaccumPumpOn)
          {
            Serial.print("[VACCUM] START TURN ON vaccum pump \n");
            digitalWrite(vacuumPumpPin, LOW);
            vaccumPumpOn = true;
          }

          for (int i = 0; i < profile.vStagesCount; i++)
          {
            bool stageReady = false;
            unsigned int vTimer = millis() / 1000;
            while (!stageReady)
            {
              int sPressure = profile.vStages[i];
              int currentSystemPressure = analogRead(systemPressurePin);

              Serial.print("[VACCUM] STAGE " + String(i) + " TIME LEFT " + String((vTimer + profile.vStagesTime[i] * 60) - millis() / 1000) + "s Sys: " + String(currentSystemPressure) + " Stage: " + String(sPressure) + "\n");
              if (currentSystemPressure > sPressure + profile.gistVac)
              {
                if (!vaccumPumpOn)
                {
                  Serial.print("[VACCUM] STAGE " + String(i) + " TURN ON vaccum pump \n");
                  digitalWrite(vacuumPumpPin, LOW);
                  vaccumPumpOn = true;
                }
              }

              if (analogRead(systemPressurePin) <= sPressure)
              {
                if (vaccumPumpOn)
                {

                  Serial.print("[VACCUM] STAGE " + String(i) + " TURN OFF vaccum pump \n");
                  digitalWrite(vacuumPumpPin, HIGH);
                  vaccumPumpOn = false;
                }
              }

              if ((vTimer + profile.vStagesTime[i] * 60) < millis() / 1000)
              {
                stageReady = true;
                Serial.print("[VACCUM] STAGE END: Sys: " + String(currentSystemPressure) + "\n");
              }
            }
          }

          vReady = true;
          Serial.print("[VACCUM] END \n");
          Serial.print("[VACCUM] TURN OFF vaccum pump \n");
          digitalWrite(vacuumPumpPin, HIGH);
          vaccumPumpOn = false;

          if (vacuumVentOpened)
          {

            Serial.print("[VACCUM] CLOSE vaccum vent \n");
            CloseVent(vacuumVentClosePin);
            vacuumVentOpened = false;
          }

          Serial.print("[DISCHARGE] OPEN OUT VENT \n");
          OpenVent(outVentOpenPin);

          Serial.print("[DISCHARGE] CLOSE OUT VENT \n");
          CloseVent(outVentClosePin);
        }

        // 2. поддержание давления на profile.pTime

        unsigned long pTimer = millis() / 1000;
        bool pReady = false;
        while (!pReady)
        {
          if (!pressureVentOpened)
          {

            Serial.print("[PRESSURE] OPEN PRESSURE VENT \n");
            OpenVent(pressureVentOpenPin);
            pressureVentOpened = true;
          }
          int currentSystemPressure = analogRead(systemPressurePin);

          Serial.print("[PRESSURE] TIME LEFT " + String((pTimer + profile.pTime * 60) - millis() / 1000) + "s Sys: " + String(currentSystemPressure) + "\n");
          if ((pTimer + profile.pTime * 60) < millis() / 1000)
          {
            pReady = true;
            Serial.print("[PRESSURE] END \n");
            if (pressureVentOpened)
            {

              Serial.print("[PRESSURE] CLOSE PRESSURE VENT \n");
              CloseVent(pressureVentClosePin);
              pressureVentOpened = false;
            }
          }
        }

        // 3. выравние по атмосфере

        Serial.print("[DISCHARGE] OPEN OUT VENT. WAIT 120s FOR DISCHARGE\n");
        OpenVent(outVentOpenPin, 8); // приоткрываем вентиляцию
        delay(120000);               // 120 секунд на плавный спуск давления
        digitalWrite(profile.ledPin, LOW);
      }
    }
    end = true;
    CloseVent(outVentClosePin);
    CloseVent(vacuumVentClosePin);
    CloseVent(pressureVentClosePin);

    while (end)
    {
      digitalWrite(fin, HIGH);
      digitalWrite(statusLedPin, LOW);
    }
  }
}
