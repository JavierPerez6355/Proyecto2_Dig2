#include <Arduino.h>
#include <Wire.h>

#define DIRECCION_I2C 0x28
#define PIN_SDA 21
#define PIN_SCL 22


void enviarDato();

uint16_t valor = 1234;

uint8_t byteAlto = valor >> 8;
uint8_t byteBajo = valor & 0xFF;


void setup() {

Serial.begin(115200);

Wire.onRequest(enviarDato);

bool iniciado = Wire.begin(
    (uint8_t)DIRECCION_I2C,
    PIN_SDA,
    PIN_SCL,
    100000
);

if (iniciado)
{
    Serial.println("ESP32 iniciado como esclavo I2C");
}
else
{
    Serial.println("Error al iniciar I2C");
}
}

void loop() {
  // put your main code here, to run repeatedly:
}

void enviarDato()
{
    Wire.write(byteAlto);
    Wire.write(byteBajo);
}
