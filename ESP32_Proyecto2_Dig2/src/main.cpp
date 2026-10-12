#include <Arduino.h>
#include <Wire.h> //proporciona la comunicación I2C
#include <LiquidCrystal.h> //proporciona la comunicación con el LCD
#include <ESP32SPISlave.h> //proporciona la comunicación SPI esclavo

//******************************************/
// Definiciones
//******************************************/
#define LED1 26 //rojo
#define LED2 2 //amarillo
#define LED3 15 //verde

#define DIRECCION_I2C 0x28 // Dirección del esclavo I2C
#define pin_SDA 21 //datos
#define pin_SCL 22 //reloj
#define pin_Pot 4 //pin del potenciómetro

#define rs 32 //indica si se envía un comando o un dato al LCD
#define en 33 //indica cuando el LCD debe leer los datos
#define d4 27 //transportan la información de 4 bits al LCD
#define d5 14
#define d6 25
#define d7 13


//******************************************/
//configuración SPI
//******************************************/
#define SPI_HEADER  0xA5   // Encabezado para validar la trama
#define BUFFER_SIZE 4      // Trama: [0xA5][LED][TIEMPO_H][TIEMPO_L]
#define QUEUE_SIZE  1      // Tamaño de la cola de mensajes

//******************************************/
//Objetos
//******************************************/
ESP32SPISlave slave;  // Crear una instancia de la clase ESP32SPISlave
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//******************************************/
// Variables SPI
//******************************************/
uint8_t tx_buf[BUFFER_SIZE] __attribute__((aligned(4))) = {0};  // Datos que se envian al maestro
uint8_t rx_buf[BUFFER_SIZE] __attribute__((aligned(4))) = {0};  // Datos que se reciben del maestro

const uint8_t ledPins[3] = {LED1, LED2, LED3}; // Array con los pines de los LEDs
const char inicialLed[3] = {'R', 'A', 'V'}; // Array con las iniciales de los LEDs

int ledActivo = -1;               // -1 = ningun LED encendido
char ultimoLed = '-';             // Almacena la inicial del último LED encendido

unsigned long tInicio = 0;        // Momento en que se encendio el LED
unsigned long tDuracion = 0;      // Tiempo que debe permanecer encendido

//******************************************/
// Variables ADC e I2C
//******************************************/

uint8_t respuesta[4] = {0, 0, 0, 0}; // Array que contiene la respuesta del esclavo I2C: [0][ADC][mV_H][mV_L]

uint16_t valorPot = 0; // Valor leído del potenciómetro
uint8_t valorADC8 = 0; // Valor del potenciómetro mapeado a 8 bits (0-255)
uint32_t milivoltios = 0; // Valor del potenciómetro en milivoltios

unsigned long tiempoAnterior = 0; // Variable para controlar el tiempo de actualización de la medición y la respuesta I2C

//******************************************/
// Prototipos
//******************************************/

void enviarDato(); // Responde a la solicitud de datos del maestro I2C
void procesarTrama(); // Lee la trama recibida por SPI y enciende el LED indicado
void apagarLeds(); // Apaga los 3 LEDs
void actualizarRespuestaI2C(); // Actualiza la medición del potenciómetro y la respuesta I2C

//******************************************/
// Configuración
//******************************************/

void setup() {

Serial.begin(115200); // Inicia la comunicación serial

//configuracion del LCD
lcd.begin(16, 2);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Iniciando...");
lcd.setCursor(0, 1);
lcd.print("ESP32");

//configuracion del ADC
analogReadResolution(12); // Configura la resolución de lectura del ADC a 12 bits (0-4095)
analogSetPinAttenuation(pin_Pot, ADC_11db); // Configura la atenuación del pin del potenciómetro a 11 dB para un rango de 0 a 3.3V

//Primera lectura del potenciómetro

actualizarRespuestaI2C();

// Configurar los 3 LEDs como salida e iniciarlos apagadaos

for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
}

//configurar SPI esclavo
slave.setDataMode(SPI_MODE0);  // Configurar el modo SPI (puede ser SPI_MODE0, SPI_MODE1, SPI_MODE2 o SPI_MODE3)
slave.setQueueSize(QUEUE_SIZE);  // Configurar el tamaño de la cola de mensajes

// Iniciar el esclavo SPI en el bus VSPI (puede ser HSPI o VSPI)
if (slave.begin(VSPI)) {
    Serial.println("ESP32 iniciado como esclavo SPI");
    Serial.print("MOSI pin: ");
    Serial.println(MOSI); 
    Serial.print("MISO pin: ");
    Serial.println(MISO);
    Serial.print("SCK pin: ");
    Serial.println(SCK);
    Serial.print("SS Pin: ");
    Serial.println(SS);
} else {
    Serial.println("Error al iniciar SPI");
}

// configuracion del esclavo I2C
Wire.onRequest(enviarDato);

// Inicia el esclavo I2C con la dirección especificada y los pines SDA y SCL
bool iniciado = Wire.begin((uint8_t)DIRECCION_I2C, pin_SDA, pin_SCL, 100000);

if (iniciado)
{
    Wire.slaveWrite(respuesta, sizeof(respuesta)); // Envía la respuesta inicial al maestro I2C
    Serial.println("ESP32 iniciado como esclavo I2C"); 
}
else
{
    Serial.println("Error al iniciar I2C");
}
}

//******************************************/
//Ciclo principal
//******************************************/

void loop()
 {

  
  // Preparar la nueva recepcion SPI si el esclavo esta libre
  if (slave.hasTransactionsCompletedAndAllResultsHandled()) 
  {
    //limpia el buffer de recepcion
    memset(rx_buf, 0, sizeof(rx_buf));
    //prepara una transaccion SPI de 4 bytes.
    slave.queue(tx_buf, rx_buf, BUFFER_SIZE);
    //habilita la transaccion. 
    slave.trigger();
  }

  //Procesar la comunicacion SPI si la transaccion ya termino y el dato ya fue recibido

  if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE)) {

    //interpreta el comando recibido

    auto bytesRecibidos = slave.numBytesReceivedAll(); // Obtiene el número de bytes recibidos en todas las transacciones completadas

    if (!bytesRecibidos.empty() && bytesRecibidos[0] == BUFFER_SIZE) { // Verifica si se recibieron 4 bytes
        procesarTrama();
    } 
    else 
    {
        Serial.println("Trama SPI incompleta");
    }
  
  }

 
  //Apagar el LED cuando se cumpla el tiempo 

  //se usa millis() para no bloquear la comunicacion SPI
  if (ledActivo >= 0 && (millis() - tInicio >= tDuracion))
   {
    digitalWrite(ledPins[ledActivo], LOW);

    Serial.print("LED ");
    Serial.print(ledActivo + 1);
    Serial.println(" apagado");

    ledActivo = -1; //ningun LED encendido
  }
    //Actualizar ADC, I2C y LCD cada 100 ms

    unsigned long tiempoActual = millis(); // Obtener el tiempo actual en milisegundos

    if (tiempoActual - tiempoAnterior >= 100) // Verificar si han pasado 100 ms desde la última actualización
    {
        tiempoAnterior = tiempoActual; // Actualizar el tiempo anterior

        actualizarRespuestaI2C(); // Actualizar la medición del potenciómetro y la respuesta I2C
        Wire.slaveWrite(respuesta, sizeof(respuesta)); // Enviar la respuesta al maestro I2C

        Serial.printf("ADC: %u\n", (unsigned int)valorADC8); 
        Serial.printf("mV: %u\n", (unsigned int)milivoltios);

        char linea1[17]; // Variable para almacenar la primera línea del LCD
        char linea2[17]; // Variable para almacenar la segunda línea del LCD

        snprintf(linea1, sizeof(linea1), "P1:%lu.%03luV", (unsigned long)(milivoltios / 1000), (unsigned long)(milivoltios % 1000));
        snprintf(linea2, sizeof(linea2), "ADC:%4u LED:%c", (unsigned int)valorADC8, ultimoLed); 

        lcd.setCursor(0, 0);
        lcd.print(linea1);
        lcd.print("      ");

        lcd.setCursor(0, 1);
        lcd.print(linea2);
        lcd.print("  ");
    }
  }

//******************************************/
// Actualizar medición y respuesta I2C
//******************************************/

void actualizarRespuestaI2C()
{
        valorPot = analogRead(pin_Pot); 
        valorADC8 = map(valorPot, 0, 4095, 0, 255); // Mapea el valor del potenciómetro a un rango de 0 a 255
        milivoltios = analogReadMilliVolts(pin_Pot); // Convierte el valor del potenciómetro a milivoltios

        respuesta[0] = 0; 
        respuesta[1] = valorADC8; // Almacena el valor del potenciómetro mapeado a 8 bits en la respuesta I2C
        respuesta[2] = (uint8_t)(milivoltios >> 8); // Almacena los bits más significativos de los milivoltios en la respuesta I2C
        respuesta[3] = (uint8_t)(milivoltios & 0xFF); // Almacena los bits menos significativos de los milivoltios en la respuesta I2C
}



//******************************************/
// Otras funciones
//******************************************/

// Responde a la solicitud de datos del maestro I2C

void enviarDato()
{
    Wire.write(respuesta, sizeof(respuesta)); // Envía la respuesta al maestro I2C
}

// Revisa la trama recibida y enciende el LED indicado
void procesarTrama() {
  //verificar el encabezado
  if (rx_buf[0] != SPI_HEADER) { // Verifica si el encabezado es correcto
    Serial.println("Trama invalida: encabezado incorrecto");
    return;
  }

  uint8_t led = rx_buf[1]; // Obtiene el número de LED del segundo byte de la trama
  uint16_t tiempo = ((uint16_t)rx_buf[2] << 8) | rx_buf[3]; // Combina los bytes 2 y 3 de la trama para obtener el tiempo en milisegundos

  //Muestra el dato recibido.
  Serial.print("Dato recibido -> LED: ");
  Serial.print(led);
  Serial.print("  Tiempo: ");
  Serial.print(tiempo);
  Serial.println(" ms");

  //verificar que los datos sean validos
  if (led < 1 || led > 3 || tiempo == 0) 
  {
    Serial.println("Trama invalida: datos incorrectos");
    return;
  }

  //encender el LED indicado
  apagarLeds();

  ledActivo = led - 1; // Ajusta el índice del LED activo (0, 1 o 2)
  ultimoLed = inicialLed[ledActivo]; // Actualiza la inicial del último LED encendido
  tDuracion = tiempo; // Establece la duración del encendido del LED
  tInicio = millis(); // Registra el momento en que se encendió el LED

  digitalWrite(ledPins[ledActivo], HIGH); // Enciende el LED correspondiente

  Serial.print("Encendiendo LED ");
  Serial.println(led);
  Serial.print("durante");
  Serial.print(tiempo);
  Serial.println(" ms");
}
//*************************************/
// Apagar todos los LEDs
//*************************************/
// Apaga los 3 LEDs
void apagarLeds(void) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  ledActivo = -1; //ningun LED encendido
}