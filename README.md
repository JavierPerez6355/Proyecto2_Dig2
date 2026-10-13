# Proyecto 2 - Sistemas embebidos

Este proyecto implementa la comunicación entre una Nucleo-F446RE y un ESP32 utilizando los protocolos SPI e I2C.

La Nucleo funciona como el controlador principal. Mediante una terminal serial, el usuario puede seleccionar entre controlar los LEDs conectados al ESP32 o solicitar la medición de un potenciómetro.

## Funcionamiento

La opción 1 permite ingresar el número de LED y el tiempo que debe permanecer encendido. Por ejemplo, el comando `1,500` enciende el LED 1 durante 500 ms. La Nucleo transmite este comando al ESP32 mediante SPI.

El mismo tiempo se utiliza para configurar una señal cuadrada generada por TIM2 en el pin PA0. Para el comando anterior, la señal permanece 500 ms en alto y 500 ms en bajo.

La opción 2 solicita al ESP32 el valor del potenciómetro. El ESP32 realiza la lectura mediante su ADC, la convierte a un intervalo de 0 a 255 y calcula el voltaje en milivoltios. Después envía ambos valores a la Nucleo mediante I2C.

El ESP32 también muestra en una pantalla LCD 16×2 el valor del ADC, el voltaje y la inicial del último LED activado.

## Comunicación

La comunicación UART se utiliza para el menú y los mensajes mostrados en la computadora.

SPI se utiliza para enviar desde la Nucleo hacia el ESP32 el número del LED y el tiempo de encendido. La trama contiene un encabezado `0xA5`, el número del LED y los dos bytes del tiempo.

I2C se utiliza para enviar desde el ESP32 hacia la Nucleo el valor del ADC y el voltaje. El ESP32 funciona con la dirección I2C `0x28`.

## Control de tiempo

Los LEDs del ESP32 se controlan mediante `millis()`, evitando el uso de retardos bloqueantes.

La señal cuadrada de la Nucleo se genera mediante TIM2 en modo Toggle. De esta manera, la señal puede continuar funcionando mientras se utilizan las comunicaciones UART, SPI e I2C.

## Hardware utilizado

- Nucleo-F446RE
- ESP32
- Pantalla LCD 16×2
- Potenciómetro
- Cuatro LEDs
- Resistencias
- Protoboard y cables de conexión

## Software utilizado

- STM32CubeIDE
- STM32CubeMX
- Visual Studio Code
- PlatformIO
- Framework Arduino para ESP32

## Uso

1. Cargar el programa del ESP32 mediante PlatformIO.
2. Cargar el programa de la Nucleo mediante STM32CubeIDE.
3. Reiniciar primero el ESP32 y después la Nucleo.
4. Abrir el monitor serial de la Nucleo a 115200 baudios.
5. Seleccionar una opción del menú.
6. Para controlar un LED, ingresar un comando con el formato `LED,TIEMPO`, por ejemplo `1,500`.

## Autores

- Nombre: Javier Pérez
- Carné: 24740
  
- Nombre: Alysson Galindo
- Carné: 231970
  
- Curso: Electrónica Digital II
- Sección: 10

## Video

Enlace al video de demostración: https://canva.link/ew9bchzdf2lltu3

## Documentación

La documentación del proyecto incluye el análisis, las conexiones utilizadas, el diagrama de flujo, los resultados obtenidos y la explicación del código.
