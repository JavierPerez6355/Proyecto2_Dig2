#include <Arduino.h>
#include <Wire.h>

#define DIRECCION_I2C 0x28
#define pin_SDA 21
#define pin_SCL 22
#define pin_Pot 34

uint8_t respuesta[2] = {0, 0};


void enviarDato();


void setup() {

Serial.begin(115200);
analogReadResolution(12);

Wire.onRequest(enviarDato);

bool iniciado = Wire.begin((uint8_t)DIRECCION_I2C, pin_SDA, pin_SCL, 100000);

if (iniciado)
{
    Wire.slaveWrite(respuesta, 2);
    Serial.println("ESP32 iniciado como esclavo I2C");
}
else
{
    Serial.println("Error al iniciar I2C");
}
}

void loop() {
  uint16_t valorPot = analogRead(pin_Pot);

  respuesta[0] = (uint8_t)(valorPot >> 8); // byte alto
  respuesta[1] = (uint8_t)(valorPot & 0xFF); // byte bajo

  Wire.slaveWrite(respuesta, 2);

  Serial.printf("ADC: %d\n", (unsigned int)valorPot);
  delay(500);
}

void enviarDato()
{
    Wire.write(respuesta, 2);
}
