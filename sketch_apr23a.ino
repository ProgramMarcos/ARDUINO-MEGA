#include <Adafruit_MCP4725.h>
#include <Wire.h>
#include "C:\'direccion del archivo\HELPER.H"  //LA DIRECCION DEBE SER MODIFICADA EN FUNCION DE 
                                                                                //DONDE SE GUARDE EL HELPER.H
//#include <tca9548A>
#define mux_ADDRESS 0x70 // Dirección I2C del multiplexor TCA9548A

// Direcciones de los DAC MCP4725
#define DAC_ADDRESS_1 0x60
#define DAC_ADDRESS_2 0x61


// Canal del multiplexor al que están conectados los DAC
#define MUX_CHANNEL_DAC01 0
#define MUX_CHANNEL_DAC23 1
#define MUX_CHANNEL_DAC45 2
#define MUX_CHANNEL_DAC67 3
#define MUX_CHANNEL_DAC89 4
#define MUX_CHANNEL_DAC1011 5



// Configurar los objetos para los DAC
Adafruit_MCP4725 dac1;
Adafruit_MCP4725 dac2;


const float tempValues[] = {-40, 40, 60, 80, 100}; 
const float Consigna[] = {1.51, 1.409, 1.409, 1.19, 1.03};   // Valores de PWM correspondientes 95%, 100%, 90%, 75%, 50%

void resetArduino();

 
float ntcRead(int Pin) {// NTC translation to Temperature (ºC)
  float R_ntc, ln_Rntc, T_K, T_C, V_in, ADC_R;
  ADC_R = Pin;
  if (ADC_R > 0.0) {
    V_in = (5.0 * ADC_R / 1023.0); // ADC reading
    R_ntc = NTC_RPU * (5.0 - V_in)/V_in; // NTC current value calculation
	}
  else {
    V_in = 0;
    R_ntc = 0.01;
  }
  // Steinhart-Hart equation 1/T = A+ B* lnR + C*(lnR)^3 [T=K, R=ohm, A/B/C constants) https://en.wikipedia.org/wiki/Steinhart%E2%80%93Hart_equation 

  ln_Rntc = log(R_ntc);
  T_K = 1.0 / (SHC_A + (SHC_B * ln_Rntc) + (SHC_C * Pow3(ln_Rntc))); // Temp in K
  T_C = ToCelcius(T_K); // Temp in celsius
  
  return T_C;
}
float calculateDerating(float temperature, float optionalVoltage = -1.0) {
  float voltage;

  if (optionalVoltage != -1.0) {
    voltage = optionalVoltage; // Utiliza el valor opcional si se proporciona
  } else {
    if (temperature <= tempValues[0]) { voltage = Consigna[0]; } // -40°C
    else if (temperature <= tempValues[1]) { voltage = Consigna[1]; } // 40°C
    else if (temperature <= tempValues[2]) { voltage = Consigna[2]; } // 60°C
    else if (temperature <= tempValues[3]) { voltage = Consigna[3]; } // 80°C
    else if (temperature <= tempValues[4]) { voltage = Consigna[4]; } // 100°C
    else { voltage = 2.0; } // Fuera de rango, establece el voltaje en 2.0V
  }

  uint16_t dacValue = voltage * 4095 / 5; // Convertir voltaje a valor digitaL
  return dacValue;
}

void setup() {
  // Inicializar la comunicación I2C
  Wire.begin();
  Serial.begin(9600);
  
  // Inicializar el multiplexor TCA9548A
  Wire.beginTransmission(mux_ADDRESS);
  //Wire.write(1 << MUX_CHANNEL_DAC); // Seleccionar el canal del multiplexor
  Wire.write(1 << MUX_CHANNEL_DAC01);
  Wire.endTransmission();

  // Inicializar el multiplexor TCA9548A
  Wire.beginTransmission(mux_ADDRESS);
  //Wire.write(1 << MUX_CHANNEL_DAC); // Seleccionar el canal del multiplexor
  Wire.write(1 << MUX_CHANNEL_DAC23);
  Wire.endTransmission();

  
  // Inicializar los DAC
  dac1.begin(DAC_ADDRESS_1);
  dac2.begin(DAC_ADDRESS_2);

  //para pruebas, uso pines 0-6 como ED (LOW)
  pinMode(0, INPUT_PULLUP); 
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
}


float salidaDAC(uint8_t puertoIN,int canalMUX){
  int value = (analogRead(puertoIN));
  float tem=ntcRead(value);
  uint16_t volta= calculaDerating(tem);
  Wire.beginTransmission(mux_ADDRESS);
  if (canalMux>5){canalMux%=5;} //si el canal es mayor que 5 lo divides entre 5 y sacas el resto, así nunca es mayor que 5
  if (canalMux==0){Wire.write( 1 <<MUX_CHANNEL_DAC01);}
  else if (canalMux==1){Wire.write( 1 <<MUX_CHANNEL_DAC23);}
  else if (canalMux==2){Wire.write( 1 <<MUX_CHANNEL_DAC45);}
  else if (canalMux==3){Wire.write( 1 <<MUX_CHANNEL_DAC67);}
  else if (canalMux==4){Wire.write( 1 <<MUX_CHANNEL_DAC89);}
  else(canalMux==5){Wire.write( 1 <<MUX_CHANNEL_DAC1011);}

  Wire.endTransmission();

  dac1.setVoltage(volta, false);
  dac2.setVoltage(volta, false); 
  return tem
}

void loop() {


  int rawValue = (analogRead(A0));
  // Leer el valor de temperatura del pin A0
  float temp = ntcRead(rawValue);
  // Calcular el valor de DAC basado en la temperatura
  uint16_t volt = calculateDerating(temp);


  Wire.beginTransmission(mux_ADDRESS);
  Wire.write( 1 <<MUX_CHANNEL_DAC01);
  Wire.endTransmission();
 // Actualizar los primeros dos DAC con el valor de voltaje
  dac1.setVoltage(volt, false);
  dac2.setVoltage(volt, false); 

  int rawValue1 = (analogRead(A0));
  // Leer el valor de temperatura del pin A0
  float temp1 = ntcRead(rawValue1);
  // Calcular el valor de DAC basado en la temperatura
  uint16_t volt1 = calculateDerating(temp1,2);


  
  // Enviar el valor de voltaje al multiplexor para los nuevos DAC
  Wire.beginTransmission(mux_ADDRESS);
  Wire.write(1 << MUX_CHANNEL_DAC23); // Seleccionar el siguiente canal del multiplexor
  Wire.endTransmission();

  // Actualizar los nuevos DAC con el valor de voltaje
  dac1.setVoltage(volt1, false);
  dac2.setVoltage(volt1, false);

  Serial.print("Temp: ");
  Serial.println(temp);
  Serial.print("Temp1: ");
  Serial.println(temp1);
  Serial.println("______________");
  
  // Esperar un tiempo antes de la próxima actualización
  delay(1000);
}