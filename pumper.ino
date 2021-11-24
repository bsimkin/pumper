typedef struct
{
  int vStages[5];       // pressure
  float vStagesTime[5]; // minutes
  int gistVac;          // pressure delta
  float pTime;          // minutes
  int cycles;
} ProfileStruct;

int systemPressurePin = A2;
int vacuumPressurePin = A1;

int pr1 = 24;
int pr2 = 26;
int fin = 28;
int statusLedPin = 22;

bool vacuumVentOpened = false;
bool pressureVentOpened = false;

bool vaccumPumpOn = false;

int outVentOpenPin = 23;
int outVentClosePin = 25;
int vacuumVentOpenPin = 27;
int vacuumVentClosePin = 29;
int pressureVentOpenPin = 31;
int pressureVentClosePin = 33;
int vacuumPumpPin = 35;

int tumblerUpPin = 50;
int tumblerDownPin = 52;

bool start = false;
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
  pinMode(pr1, OUTPUT);
  pinMode(pr2, OUTPUT);
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
  ProfileStruct p[3];

  // ПРОФИЛЬ 1
  p[0].vStages[0] = 88; // давление
  p[0].vStages[1] = 72;
  p[0].vStages[2] = 60;
  p[0].vStages[3] = 47;
  p[0].vStages[4] = 33;

  p[0].vStagesTime[0] = 5; // время в минутах
  p[0].vStagesTime[1] = 20;
  p[0].vStagesTime[2] = 30;
  p[0].vStagesTime[3] = 30;
  p[0].vStagesTime[4] = 40;

  p[0].gistVac = 5; // разница давлений
  p[0].pTime = 40;  // время удержания давления в минутах
  p[0].cycles = 1;  // количество циклов

  // ПРОФИЛЬ 2
  p[1].vStages[0] = 60; // давление
  p[1].vStages[1] = 33;
  p[1].vStages[2] = 60;
  p[1].vStages[3] = 33;
  p[1].vStages[4] = 60;
  p[1].vStages[5] = 33;

  p[1].vStagesTime[0] = 5; // время в минутах
  p[1].vStagesTime[1] = 30;
  p[1].vStagesTime[2] = 5;
  p[1].vStagesTime[3] = 30;
  p[1].vStagesTime[4] = 5;
  p[1].vStagesTime[5] = 30;

  p[1].gistVac = 5; // разница давлений
  p[1].pTime = 20;  // время удержания давления в минутах
  p[1].cycles = 1;  // количество циклов

  while (!start)
  {

    digitalWrite(statusLedPin, HIGH);
    if (digitalRead(tumblerUpPin) == 0)
    {
      profile = p[0];
      digitalWrite(pr1, HIGH);

      start = true;
      Serial.print("[START] PROFILE: 1\n");
    }
    if (digitalRead(tumblerDownPin) == 0)
    {
      profile = p[1];
      digitalWrite(pr2, HIGH);

      start = true;
      Serial.print("[START] PROFILE: 2\n");
    }
    Serial.print("----\n");

    delay(2000);
  }
  while (start)
  {
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
        // посчитать время всех стадий
        for (int i = 0; i < sizeof profile.vStages / sizeof profile.vStages[0]; i++)
        {
          bool stageReady = false;
          unsigned int vTimer = millis() / 1000;
          while (!stageReady)
          {

            delay(1000); // ждём секунду, чтобы не сильно засирать логи

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
    }

    start = false;
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
