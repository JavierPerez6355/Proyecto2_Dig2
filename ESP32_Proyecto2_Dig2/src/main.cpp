#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define DIRECCION_I2C 0x28
#define pin_SDA 21
#define pin_SCL 22
#define pin_Pot 4

#define rs 32
#define en 33
#define d4 27
#define d5 14
#define d6 25
#define d7 13

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int contador = 0;

uint8_t respuesta[4] = {0, 0, 0, 0};

uint16_t valorPot = 0;
uint32_t milivoltios = 0;
unsigned long tiempoAnterior = 0;


void enviarDato();


void setup() {


lcd.begin(16, 2);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Prueba LCD");
lcd.setCursor(0, 1);
lcd.print("ESP32 iniciado");

Serial.begin(115200);
analogReadResolution(12);
analogSetPinAttenuation(pin_Pot, ADC_11db);



Wire.onRequest(enviarDato);

bool iniciado = Wire.begin((uint8_t)DIRECCION_I2C, pin_SDA, pin_SCL, 100000);

if (iniciado)
{
    Wire.slaveWrite(respuesta, 4);
    Serial.println("ESP32 iniciado como esclavo I2C");
}
else
{
    Serial.println("Error al iniciar I2C");
}
}

void loop()
{
    unsigned long tiempoActual = millis();

    if (tiempoActual - tiempoAnterior >= 100)
    {
        tiempoAnterior = tiempoActual;

        valorPot = analogRead(pin_Pot);
        milivoltios = analogReadMilliVolts(pin_Pot);

        respuesta[0] = (uint8_t)(valorPot >> 8);
        respuesta[1] = (uint8_t)(valorPot & 0xFF);
        respuesta[2] = (uint8_t)(milivoltios >> 8);
        respuesta[3] = (uint8_t)(milivoltios & 0xFF);

        Wire.slaveWrite(respuesta, sizeof(respuesta));

        Serial.printf("ADC: %u\n", (unsigned int)valorPot);
        Serial.printf("mV: %u\n", (unsigned int)milivoltios);

        char linea1[17];
        char linea2[17];

        snprintf(linea1, sizeof(linea1), "P1:%lu.%03luV", (unsigned long)(milivoltios / 1000), (unsigned long)(milivoltios % 1000));
        snprintf(linea2, sizeof(linea2), "ADC:%4u LED:-", (unsigned int)valorPot);

        lcd.setCursor(0, 0);
        lcd.print(linea1);
        lcd.print("      ");

        lcd.setCursor(0, 1);
        lcd.print(linea2);
        lcd.print("  ");
    }
}

void enviarDato()
{
    Wire.write(respuesta, 4);
}
