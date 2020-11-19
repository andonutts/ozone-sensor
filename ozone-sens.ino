/* ozone-sens.ino
 *  by Andy He
 */

float zeroSensor(int durationSec);
float getConcentration(int durationSec);

// fixed numerical constants
const float V_CC = 5.0;
const float V_SUP = 3.3;
const float V_REF = V_SUP * 1000000.0 / (1000000.0 + 16200.0 + 1000000.0) * 1000.0;
const float T_SPAN = 87.0;
const float T_OFFSET = 18.0;

// ozone trans-impedance amplifier gain
const long O3_TIA_GAIN = 499000;

// temperature span correction coefficient
const float O3_TEMP_C = 0.25;

// temperature baseline correction coefficients
const float O3_A_HI = 0.04;
const float O3_A_LO = 0.02;

// sensitivity code (nA/ppm)
const float SENS_CODE = -50.83;

// analog pins connected to sensor module
int vgasPin = A0;
int vtempPin = A3;

// sensor calibration values
float voffset = 0;
float tzero = 20;

void setup() {
  Serial.begin(9600);
  Serial.println("PROGRAM START");
  Serial.println("Zeroing ozone sensor...");

  zeroSensor(10);
  
  Serial.print("voffset = ");
  Serial.print(voffset);
  Serial.print(" mV");
  Serial.println();
  
  Serial.print("tzero = ");
  Serial.print(tzero);
  Serial.print(" degrees C");
  Serial.println();
}

void loop() {
  float conc = getConcentration(5);
  Serial.print(conc);
  Serial.println("ppm O3");
}

// takes the average Vgas and Vtemp voltages over durationSec seconds, then
// sets Voffset and Tzero to those values, effectively zeroing the sensor
float zeroSensor(int durationSec) {
  unsigned long startMillis;
  unsigned long durationMs = durationSec * 1000;
  unsigned long gasSum = 0, tempSum = 0;
  unsigned int n = 0;

  startMillis = millis();
  do {
    gasSum += analogRead(vgasPin);
    tempSum += analogRead(vtempPin);
    n++;
    delay(1);
  } while((millis() - startMillis) < durationMs);

  float gasAvg = (float)gasSum / (float)n;
  float vgas = gasAvg * V_CC * 1000.0 / 1024.0;
  voffset = vgas - V_REF;

  float tempAvg = (float)tempSum / (float)n;
  float vtemp = tempAvg * V_CC / 1024.0;
  tzero = (T_SPAN / V_SUP) * vtemp - T_OFFSET;

  return voffset;
}

// computes the average ozone concentration over durationSec seconds
float getConcentration(int durationSec) {
  unsigned long startMillis;
  unsigned long durationMs = durationSec * 1000;
  unsigned long gasSum = 0, tempSum = 0;
  unsigned int n = 0;

  startMillis = millis();
  do {
    gasSum += analogRead(vgasPin);
    tempSum += analogRead(vtempPin);
    n++;
    delay(1);
  } while((millis() - startMillis) < durationMs);

  float gasAvg = (float)gasSum / (float)n;
  float vgas = gasAvg * V_CC * 1000.0 / 1024.0;

  float tempAvg = (float)tempSum / (float)n;
  float vtemp = tempAvg * V_CC / 1024.0;
  float temp = (T_SPAN / V_SUP) * vtemp - T_OFFSET;

  float nA = (vgas - V_REF - voffset) / (float)O3_TIA_GAIN * 1000000.0;

  // temperature compensation
  if (temp > 23.0) {
    nA -= O3_A_HI * (temp - tzero);
  } else {
    nA -= O3_A_LO * (temp - tzero);
  }

  return nA / SENS_CODE * (1 - O3_TEMP_C * (temp - tzero));
}
