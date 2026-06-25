# lab-8-arduino-uno-rtos-tasks

## Parte 1: FreeRTOS

### ¿Qué es un RTOS?

Un Sistema Operativo de Tiempo Real (RTOS, por sus siglas en inglés, *Real-Time Operating System*) es un tipo de software de sistema diseñado para ser pequeño, ligero y altamente determinista. A diferencia de los sistemas operativos de propósito general (como Windows, Linux o Android), que buscan repartir el tiempo del procesador de forma "justa" entre múltiples usuarios y programas grandes, un RTOS está enfocado en cumplir con restricciones de tiempo muy estrictas y ofrecer un control fino y preciso sobre el hardware.

Estos sistemas se utilizan comúnmente en dispositivos embebidos y microcontroladores donde es crítico reaccionar a eventos externos de manera predecible (por ejemplo, dispositivos médicos, unidades de control automotriz o sistemas robóticos). En un RTOS como FreeRTOS, el proyecto se divide en pequeñas unidades lógicas independientes llamadas "tareas" (*tasks*). El núcleo del sistema operativo (*kernel*) utiliza un "planificador" (*scheduler*) para alternar rápidamente la ejecución de estas tareas, creando la ilusión de que todas se ejecutan en paralelo al mismo tiempo, incluso si el microcontrolador posee un solo núcleo.

### Bare Metal vs RTOS

La programación tradicional en microcontroladores, como la que se utiliza por defecto en la plataforma Arduino, se conoce como un enfoque **Bare-Metal** (código ejecutado directamente sobre el hardware sin la ayuda de un sistema operativo).

Características de la programación en **Bare-Metal**:

* **Flujo secuencial:** Todas las instrucciones de la aplicación se ejecutan de manera lineal dentro de un único bucle principal, como la función `loop()` en Arduino.
* **Bloqueo del procesador:** Si el programa utiliza comandos de retardo, como `delay()`, el microcontrolador se detiene por completo y espera el tiempo indicado sin ejecutar ninguna otra acción.
* **Una sola tarea a la vez:** Debido a los retardos bloqueantes, mientras el procesador espera, nada más puede ejecutarse en el sistema, lo que vuelve ineficiente el manejo de varios componentes simultáneamente.

Por el contrario, al utilizar un **RTOS**:

* **El proyecto se divide en módulos:** Cada característica o rutina (leer un sensor, mover un motor, imprimir en pantalla) se convierte en una tarea independiente.
* **No hay bloqueos totales de CPU:** En lugar de usar retardos que bloquean todo el procesador, las tareas utilizan funciones de retardo no bloqueantes propias del RTOS (como `vTaskDelay()`). Si una tarea necesita esperar tiempo o datos, el planificador le quita el control de la CPU y se la entrega inmediatamente a otra tarea que esté lista para trabajar.
* **Transiciones fluidas:** El *scheduler* se encarga de gestionar los tiempos y cambiar de tareas de forma tan rápida y suave que el sistema logra una verdadera multitarea concurrente, mejorando la confiabilidad del código.

---

### Conceptos base para la programación en FreeRTOS

Para poder escribir programas utilizando un RTOS, es necesario entender las piezas fundamentales que lo componen y cómo interactúan entre sí para gestionar los recursos del microcontrolador.

* **Scheduler (Planificador):** Es el componente central del núcleo del sistema operativo. Se encarga de decidir qué tarea debe ejecutarse en el procesador en cada instante de tiempo. FreeRTOS utiliza un planificador de tipo **preemptive (preventivo o con apropiación)**. Esto significa que la tarea con la prioridad más alta que esté lista para ejecutarse obtendrá el control de la CPU inmediatamente, interrumpiendo (preempting) a cualquier otra tarea de menor prioridad que se esté ejecutando en ese momento.
* **Tasks (Tareas):** En FreeRTOS, una tarea es un pequeño programa independiente (generalmente un bucle infinito) que se encarga de una funcionalidad específica, como leer un sensor o hacer parpadear un LED. Cada tarea tiene las siguientes propiedades:
  * **Priority (Prioridad):** Un valor numérico que determina la importancia de la tarea. El *scheduler* utiliza este valor para decidir quién obtiene la CPU. A mayor número, mayor es la prioridad de la tarea.
  * **Stack memory (Memoria de Pila):** Cada tarea posee su propio espacio de memoria (stack) reservado para guardar sus variables locales y el contexto del procesador (los valores de los registros) cuando el *scheduler* la pausa para ejecutar otra tarea.
  * **State (Estado):** Una tarea no siempre está usando el procesador. En su ciclo de vida, transita por cuatro estados principales:

```text
                            Creación de tarea
                                    |
                                    v
                          +-------------------+
                          |                   |
            +------------ | LISTA / DISPONIBLE| <---------------+
            |             |     (READY)       |                 |
            |             +-------------------+                 |
            |               |               ^                   |
            |   El Scheduler|               | Cambio de         | Evento 
            |   asigna la   |               | contexto          | (dato,
Pausada     |   CPU a la    |               | (Ej. fin de tick) | timeout)
manualmente |   tarea       v               |                   | 
(Suspend)   |             +-------------------+                 |
            |             |                   |                 |
            +-----------> |    EJECUTANDO     |                 |
            |             |     (RUNNING)     |                 |
            |             +-------------------+                 |
            |                       |                           |
            |                       | Espera un evento          |
            |                       | (delay, cola, mutex)      |
            |                       v                           |
            |             +-------------------+                 |
            |             |                   |                 |
            |             |     BLOQUEADA     | ----------------+
            |             | (WAITING/BLOCKED) |
            |             +-------------------+
            v

  +-------------------+
  |                   |
  |    SUSPENDIDA     | ----> Reactivada (Resume) pasa a READY
  |    (SUSPENDED)    |
  +-------------------+
```

* **Delays (Retardos no bloqueantes):** A diferencia de las demoras tradicionales que congelan la CPU, FreeRTOS utiliza funciones como `vTaskDelay()`. Cuando una tarea invoca este retardo, pasa inmediatamente al estado **Bloqueada (Blocked)**, cediendo el procesador para que otras tareas puedan ejecutarse mientras transcurre el tiempo especificado.
* **Queues (Colas):** Son estructuras de datos FIFO (First-In, First-Out) diseñadas para la comunicación segura entre tareas. Permiten que una tarea envíe información (como la lectura de un sensor) y otra tarea la reciba, evitando la corrupción de datos que ocurre cuando múltiples tareas intentan acceder a una misma variable global.
* **Semaphores and Mutexes (Semáforos y Mutexes):** Son mecanismos para la sincronización y la gestión de recursos compartidos.
  * **Binary Semaphores (Semáforos binarios):** Se usan principalmente para sincronizar tareas o interrupciones. Una tarea puede esperar (bloquearse) hasta que otra tarea o un evento de hardware "dé" la señal a través del semáforo de que una acción ha ocurrido.
  * **Mutexes (Exclusión mutua):** Se utilizan para proteger un recurso compartido (como un puerto de comunicación serie o un bus I2C). Solo la tarea que posee el "token" (mutex) puede acceder al recurso. Las demás tareas deben esperar a que se libere, previniendo conflictos y colisiones.

### ¿Entonces cómo funciona FreeRTOS?

Para entender cómo funciona FreeRTOS en conjunto, debemos ver cómo interactúan los conceptos anteriores. Tu aplicación se divide en múltiples pequeños programas independientes (tareas), cada uno con una función específica, un nivel de prioridad y su propia memoria de pila (*stack*).

Al iniciar el sistema, el *Scheduler* toma el control total del procesador. Su trabajo es evaluar constantemente qué tarea debe estar en estado **Ejecutando (Running)**. Si la tarea activa necesita esperar a que transcurra un tiempo (usando un `vTaskDelay`) o a que llegue información desde un sensor (a través de una *Queue* o *Semaphore*), esta cede voluntariamente el procesador y pasa al estado **Bloqueada (Blocked)**.

En ese instante preciso, el *Scheduler* realiza un "cambio de contexto": guarda el estado de la tarea bloqueada y le entrega inmediatamente la CPU a la siguiente tarea en estado **Disponible (Ready)** que tenga la prioridad más alta. Gracias a este enfoque preemptive, el procesador nunca se queda atascado en bucles de espera inútiles y el sistema logra reaccionar a los eventos en tiempo real, garantizando que el código más urgente siempre se ejecute primero.

Además, por diseño, siempre está corriendo por defecto una tarea base llamada **Idle Task** (Tarea inactiva). El *Scheduler* la crea automáticamente al arrancar el sistema y le asigna la prioridad más baja posible (prioridad 0). Esto garantiza que el microcontrolador siempre tenga una instrucción válida que ejecutar cuando todas las tareas de tu aplicación se encuentran bloqueadas esperando un evento, manteniendo el sistema activo o permitiéndole entrar en modos de bajo consumo.

---

### Beneficios que tiene usar un RTOS

Implementar un RTOS como FreeRTOS aporta ventajas significativas frente a la programación tradicional en *Bare-Metal*, especialmente cuando el proyecto crece en tamaño:

* **Multitarea real y código no bloqueante:** Permite ejecutar múltiples operaciones lógicas de forma concurrente. Las funciones de retardo y los mecanismos de espera de eventos evitan que un proceso detenga la ejecución de todo el microcontrolador.
* **Modularidad y estructura limpia:** Divide una aplicación compleja en un conjunto de tareas más pequeñas y manejables. Esto facilita la reutilización de código, simplifica las pruebas de software y mejora la división del trabajo en equipos de desarrollo.
* **Control de tiempo determinista:** A diferencia de los sistemas operativos de propósito general, un RTOS otorga un control muy fino y preciso sobre el tiempo, garantizando que los plazos estrictos (*deadlines*) de los eventos físicos se cumplan.
* **Gestión de recursos segura:** Mediante el uso de colas (*Queues*) y semáforos, el RTOS previene la corrupción de datos que ocurre cuando múltiples tareas intentan acceder o modificar la misma variable global o el mismo bus de comunicación (como I2C o UART) al mismo tiempo.

---

### Casos de uso en los que quieres usarlo

FreeRTOS es la herramienta ideal cuando te enfrentas a escenarios con las siguientes características:

* **Proyectos de escala media o alta:** Cuando tienes múltiples componentes de hardware (sensores, pantallas, módulos de red) que deben interactuar de forma simultánea.
* **Gestión de múltiples prioridades:** Cuando tu sistema debe atender eventos críticos de forma inmediata. Por ejemplo, en un robot, atender una alerta de colisión frontal (alta prioridad) es mucho más urgente que actualizar los datos en una pantalla LCD (baja prioridad).
* **Requisitos asíncronos y temporales distintos:** Cuando tienes rutinas que operan a frecuencias muy diferentes. Por ejemplo, un sistema donde un lazo de control de motores debe ejecutarse con precisión cada 2 milisegundos, pero al mismo tiempo el dispositivo debe transmitir telemetría por Wi-Fi cada 5 segundos de forma asíncrona.

---

### Notación y sintaxis en FreeRTOS

Una de las características visuales que más llama la atención al empezar a usar FreeRTOS es su estilo de codificación. El código fuente utiliza la **notación húngara**, una convención donde el nombre de cada variable o función está precedido por letras minúsculas que indican el tipo de dato que devuelve o maneja.

Las reglas de sintaxis más comunes son:

* **`v`** para tipos `void` (ej. `vTaskDelay`).
* **`c`** para variables tipo `char`.
* **`s`** para variables tipo `short`.
* **`l`** para variables tipo `long` (enteros de 32 bits).
* **`x`** para estructuras o tipos de datos propios de FreeRTOS, como `BaseType_t` (ej. `xTaskCreate` o `xQueueSend`).
* **`p`** se añade como prefijo para denotar un puntero (ej. `pvParameters` indica un puntero a un tipo *void*).
* **`u`** se añade como prefijo para denotar tipos sin signo o *unsigned* (ej. `uxPriority`).

Aunque este estándar pueda parecer un anacronismo hoy en día, se adoptó hace casi 20 años para asegurar la claridad en compiladores antiguos de C y se mantiene por estricta compatibilidad con miles de proyectos existentes. Al programar, simplemente debes acostumbrar la vista e ignorar estas letras iniciales para entender el propósito de la función.

#### Las constantes `pdTRUE` y `pdFALSE`

En consonancia con su estilo y su diseño original en lenguaje C (que no contaba originalmente con el tipo nativo `bool`), FreeRTOS define sus propias constantes de verdad y falsedad bajo el prefijo `pd` (**Project Definitions**):
* **`pdTRUE`**: Equivale numéricamente a `1` (Verdadero).
* **`pdFALSE`**: Equivale numéricamente a `0` (Falso).

Estas macros se utilizan como el estándar de retorno en funciones del kernel para verificar operaciones exitosas (por ejemplo, al tomar un semáforo con `xSemaphoreTake()` o recibir datos de una cola con `xQueueReceive()`).


## Parte 2: Programación

### FreeRTOS en Arduino (Arduino_FreeRTOS_Library)

Aunque FreeRTOS está diseñado para procesadores más potentes, es posible utilizarlo en placas Arduino con arquitectura AVR (como Uno, Nano, Leonardo y Mega) gracias a la biblioteca `Arduino_FreeRTOS_Library`. Esta biblioteca fue desarrollada originalmente por Richard Barry y actualmente es mantenida por Phillip Stevens y otros colaboradores. El repositorio oficial del proyecto se encuentra alojado en GitHub bajo la dirección `https://github.com/feilipu/Arduino_FreeRTOS_Library`. Una característica técnica muy original de esta adaptación es que utiliza el temporizador *Watchdog* (perro guardián) nativo del microcontrolador ATmega para generar los *ticks* del planificador, permitiendo intervalos configurables de 15 ms a 500 ms.

### Beneficios prácticos de usar FreeRTOS en Arduino

Implementar este sistema operativo en proyectos de Arduino ofrece ventajas notables para el desarrollo frente a la programación tradicional:

* **Multitarea sin bloqueos:** Permite reemplazar la función bloqueante `delay()` con retardos propios de las tareas, liberando el procesador para otras acciones.
* **Ejecución simultánea:** Facilita ejecutar múltiples operaciones lógicas a la vez, permitiendo, por ejemplo, tener una tarea haciendo parpadear un LED, otra leyendo sensores y una tercera imprimiendo datos en el puerto serie de forma concurrente.
* **Mejor control de tiempos:** Las tareas pueden ejecutarse en intervalos fijos y predecibles, lo que mejora la confiabilidad del código.
* **Estructura de proyecto más limpia:** Cada característica o componente se mantiene contenido en su propia función independiente, lo que hace que los proyectos complejos sean mucho más fáciles de mantener.
* **Ideal para IoT y tiempo real:** Funciona excelente cuando se necesita estabilidad temporal, actualizaciones periódicas o reacción a eventos asíncronos.

### Limitaciones de FreeRTOS en placas basadas en AVR

A pesar de sus grandes beneficios, el uso de FreeRTOS en placas AVR, como el Arduino Uno o el Nano, conlleva ciertas restricciones físicas importantes:

* **Memoria RAM muy reducida:** Estas placas cuentan con tan solo 2 KB de memoria RAM, lo que representa la principal limitación y cuello de botella del sistema.
* **Pilas (stacks) muy pequeñas:** Debido a la escasez de RAM, solo se pueden crear unas pocas tareas ligeras, y se debe dimensionar cuidadosamente la memoria de pila de cada una.
* **Ausencia de funciones avanzadas:** Características nativas que consumen mucha memoria, como los temporizadores de software (*software timers*), grupos de eventos complejos o colas de datos muy grandes, podrían no caber en la memoria del Arduino.
* **Solo multitarea simple:** El entorno es ideal para tareas básicas (como parpadeo de LEDs o lectura de sensores), pero el riesgo de desbordamiento de pila (*stack overflow*) aumenta drásticamente si se utilizan variables grandes o muchas tareas.

### Funciones básicas de programación

Para escribir código en el IDE de Arduino utilizando esta biblioteca, es fundamental conocer la API de FreeRTOS. A continuación, se detallan las funciones más utilizadas, agrupadas por su propósito:

**1. Gestión de Tareas (Tasks)**

* `xTaskCreate()`: Es la función principal para instanciar una nueva tarea. Asigna memoria para el TCB (*Task Control Block*) y la pila, y la añade a la lista de tareas listas para ejecutarse. Recibe parámetros como el puntero a la función, el nombre, el tamaño de la pila, los parámetros de la tarea y su prioridad.
* `vTaskStartScheduler()`: Inicia el planificador del sistema operativo, tomando el control del microcontrolador para comenzar a ejecutar las tareas creadas. *(Nota: En la biblioteca Arduino_FreeRTOS, esta función se llama automáticamente al finalizar el `setup()`, por lo que no es necesario escribirla manualmente).* 
* `vTaskDelete()`: Elimina una tarea del planificador. El sistema (a través de la *Idle Task*) se encargará de liberar automáticamente la memoria que el kernel le había asignado a esa tarea.
* `taskYIELD()`: Solicita al planificador un cambio de contexto (context switch) inmediato. Si se ejecuta desde una tarea, cede la CPU a tareas de igual prioridad. Si se ejecuta al final de una rutina de interrupción (ISR) cuando una tarea de mayor prioridad ha sido despertada, obliga a la CPU a ejecutar esa tarea inmediatamente sin esperar a que transcurra el siguiente *tick* del sistema.


**2. Retardos y Control de Tiempo (Delays)**

* `vTaskDelay()`: Pausa (bloquea) la tarea actual durante un número específico de *ticks* del sistema. A diferencia del `delay()` de Arduino, permite que otras tareas utilicen el procesador mientras transcurre el tiempo.
* `vTaskDelayUntil()`: Similar a `vTaskDelay()`, pero se utiliza cuando una tarea necesita ejecutarse con una frecuencia periódica y matemáticamente exacta. El tiempo de bloqueo se calcula de forma absoluta basándose en la última vez que la tarea se despertó, ignorando el tiempo que le tomó a la tarea ejecutar su código.
* `pdMS_TO_TICKS()`: Es una macro de gran utilidad que convierte un valor de tiempo expresado en milisegundos (entendible para humanos) a la cantidad de *ticks* del kernel equivalentes, adaptándose a la configuración de frecuencia del sistema.

**3. Comunicación mediante Colas (Queues)**

* `xQueueCreate()`: Reserva la memoria necesaria y crea una nueva cola. Requiere especificar dos parámetros: la cantidad máxima de elementos que la cola puede almacenar (longitud) y el tamaño en bytes de cada elemento.
* `xQueueSend()` / `xQueueSendToBack()`: Envía un nuevo dato copiándolo al final de la cola. Si la cola está llena, permite especificar un tiempo máximo de espera (*timeout*) para que la tarea se bloquee hasta que haya espacio disponible.
* `xQueueReceive()`: Lee y elimina el primer elemento disponible en la cola (el más antiguo). Si la cola está vacía, la tarea puede bloquearse esperando a que otra tarea envíe datos.
* `uxQueueMessagesWaiting()`: Es una función de consulta que devuelve el número de elementos que actualmente están almacenados en la cola y listos para ser leídos.

**4. Sincronización y Protección de Recursos (Semaphores & Mutexes)**

* `xSemaphoreCreateBinary()`: Crea un semáforo binario (que solo puede valer 0 o 1). Suele inicializarse vacío y se utiliza principalmente para sincronizar una tarea con una interrupción de hardware u otro evento específico.
* `xSemaphoreCreateMutex()`: Crea un Mutex (Exclusión Mutua), que es un tipo especial de semáforo binario diseñado para proteger un recurso compartido (como el puerto I2C o una variable global). El Mutex incluye un mecanismo de "herencia de prioridad" para evitar problemas de inversión de prioridades.
* `xSemaphoreTake()`: Intenta obtener ("tomar") el semáforo o mutex. Si el recurso no está disponible, la tarea entra en estado de bloqueo durante un tiempo máximo especificado hasta que el recurso sea liberado.
* `xSemaphoreGive()`: Libera ("devuelve") el semáforo o mutex que la tarea había tomado previamente. Esto cambia el estado del recurso a disponible y despierta a cualquier tarea que estuviera bloqueada esperando por él.

### Consideraciones Críticas de Programación en Arduino

Al programar con `Arduino_FreeRTOS_Library`, existen reglas de oro que difieren de la programación tradicional en Bare-Metal:

* **Inicio automático del Planificador (Scheduler):** A diferencia de FreeRTOS puro en *bare-metal* o en entornos como ESP-IDF, en Arduino no necesitas iniciar el planificador manualmente. Después de que todas las tareas se crean dentro del `setup()`, el planificador de FreeRTOS arranca automáticamente al finalizar dicha función. A partir de ese instante:
  * FreeRTOS toma el control total de la CPU.
  * El planificador nunca se detiene y gestiona el cambio de tareas continuamente en segundo plano.
  * Cada tarea se ejecuta de acuerdo con su nivel de prioridad y su temporización.
* **¿Qué pasa con la función `loop()`?:** En la programación clásica de Arduino, todo el código principal vive dentro del `loop()`. Sin embargo, cuando FreeRTOS está en ejecución, la función `loop()` ya no se utiliza. El planificador maneja el cambio de contexto automáticamente, por lo que toda la lógica de la aplicación debe reubicarse dentro de las tareas de FreeRTOS que creaste. Por esta razón, la buena práctica es dejar el `loop()` completamente **vacío** en proyectos de Arduino basados en FreeRTOS. Además, al no usar el `loop()`, el tamaño de la pila de la tarea inactiva (*Idle Task*) se puede reducir significativamente (ahorrando memoria RAM vital en el ATmega).
* **Retardos y llamadas bloqueantes:** Debido a que el cambio de tareas ocurre continuamente, **deben evitarse** a toda costa las llamadas bloqueantes tradicionales como `delay()` dentro de las tareas. En su lugar, utiliza siempre las funciones amigables con el sistema operativo como `vTaskDelay()` o `vTaskDelayUntil()`.
* **Interrupciones (ISRs - *Interrupt Service Routines*):** Es muy común en Arduino usar interrupciones de hardware (por ejemplo, al presionar un botón o recibir datos por un pin). **Nunca** debes llamar a una función normal de la API de FreeRTOS desde una interrupción. FreeRTOS proporciona versiones especiales de sus funciones que terminan con el sufijo `FromISR` para estos casos (por ejemplo, `xSemaphoreGiveFromISR()` o `xQueueSendFromISR()`). Usar las funciones normales dentro de una interrupción causará que el microcontrolador falle.
* **Manejo de Memoria (`pvPortMalloc` vs Variables Locales):** Las placas basadas en AVR, como el Arduino Uno, tienen solo 2 KB de RAM. Si necesitas crear estructuras grandes, arreglos (*arrays*) o búferes de datos dentro de una tarea, se recomienda enfáticamente asignarlos dinámicamente usando `pvPortMalloc()` del sistema operativo, en lugar de definirlos como variables locales. Definir arreglos grandes de forma local consumirá rápidamente la pequeña pila (*stack*) asignada a la tarea, provocando un desbordamiento de pila (*Stack Overflow*) y el reinicio de la placa.

---

### Evidencias luego de la implementación

Para verificar el cumplimiento de los requerimientos solicitados en la práctica, se diseñó e implementó un **Sistema Inteligente de Semáforos con Cruce Peatonal** en un microcontrolador Arduino Uno R3, utilizando la biblioteca `Arduino_FreeRTOS_Library`. 

El diseño se ideó con el propósito de ser probado y simulado en **SimulIDE**, organizando los componentes físicos y el código de la siguiente manera:

#### 1. Conexiones y Circuito en SimulIDE

* **Semáforo Vehicular:**
  * LED Rojo: Pin 12 (en serie con resistencia de 220 $\Omega$)
  * LED Amarillo: Pin 11 (en serie con resistencia de 220 $\Omega$)
  * LED Verde: Pin 10 (en serie con resistencia de 220 $\Omega$)
* **Semáforo Peatonal:**
  * LED Rojo: Pin 9 (en serie con resistencia de 220 $\Omega$)
  * LED Verde: Pin 8 (en serie con resistencia de 220 $\Omega$)
* **Botón de Solicitud de Cruce:**
  * Pin 2 (configurado en modo `INPUT_PULLUP`). Un extremo del pulsador se conecta al Pin 2 de Arduino y el otro a `GND`. Cuando es presionado, genera un flanco de bajada (FALLING) que dispara una interrupción de hardware externa (`INT0`).

#### 2. Arquitectura de Software en FreeRTOS

La lógica del programa se divide en dos tareas concurrentes de distintas prioridades y un mecanismo de interrupción para la entrada del usuario:

1. **Tarea Controladora del Ciclo de Semáforos (`controllerTask` - Prioridad 2 / Alta):**
   * Se encuentra inicialmente en estado **Bloqueado (Blocked)** a la espera del semáforo binario `xSemButton`.
   * Al ser activada por el botón a través de la interrupción, despierta inmediatamente (apropiación de CPU por prioridad), imprime su estado y ejecuta secuencialmente el cambio de luces utilizando `vTaskDelay()` para no bloquear la CPU (pila de 100 palabras / 200 bytes).
   * Cuenta con una fase de parpadeo de luz verde peatonal y un periodo de **cooldown** (enfriamiento de 6 segundos) que impide que carros se detengan indefinidamente por solicitudes peatonal repetidas.
2. **Tarea Periódica de Monitoreo (`monitorTask` - Prioridad 1 / Baja):**
   * Es una tarea periódica gestionada mediante la función `vTaskDelay()`.
   * Se ejecuta de forma regular cada **3 segundos**, imprimiendo estadísticas resumidas en el puerto serie (tiempo actual en ticks, código numérico de estado del semáforo y cantidad acumulada de solicitudes atendidas) usando una pila liviana de 150 palabras (300 bytes).
3. **Mecanismos de Sincronización:**
   * **Semáforo Binario (`xSemButton`):** Sincroniza la ISR del botón (Pin 2) con la tarea controladora de alta prioridad, despertándola de inmediato ante la acción del peatón mediante la función `xSemaphoreGiveFromISR()`.


#### 3. Simulación y Salida

Al simular la placa y abrir la consola serie, la salida esperada ilustra la alternancia limpia de tareas y la ejecución priorizada de eventos:

```text
RTOS Ready
Ticks: 186 | State: CAR_GREEN | Crossings: 0
Ticks: 372 | State: CAR_GREEN | Crossings: 0

Ticks: 558 | State: CAR_GREEN | Crossings: 0
Ticks: 744 | State: CAR_GREEN | Crossings: 0

[Crossing Requested]
Ticks: 930 | State: CAR_YELLOW | Crossings: 0
Ticks: 1116 | State: CAR_YELLOW | Crossings: 0
Ticks: 1302 | State: PED_CROSSING | Crossings: 1
Ticks: 1488 | State: PED_CROSSING | Crossings: 1
Ticks: 1674 | State: PED_CROSSING | Crossings: 1
Ticks: 1860 | State: PED_WARNING | Crossings: 1
Ticks: 2046 | State: PED_WARNING | Crossings: 1
Ticks: 2232 | State: PED_WARNING | Crossings: 1
Ticks: 2418 | State: COOLDOWN | Crossings: 1
Ticks: 2604 | State: COOLDOWN | Crossings: 1
Ticks: 2790 | State: CAR_GREEN | Crossings: 1
Ticks: 2976 | State: CAR_GREEN | Crossings: 1

[Crossing Requested]
Ticks: 3162 | State: CAR_YELLOW | Crossings: 1
Ticks: 3348 | State: CAR_YELLOW | Crossings: 1
Ticks: 3534 | State: PED_CROSSING | Crossings: 2
Ticks: 3720 | State: PED_CROSSING | Crossings: 2
Ticks: 3906 | State: PED_CROSSING | Crossings: 2
```

#### Análisis del comportamiento de la simulación

Video de prueba: [video](/media/arduino-uno-rtos-simple.mp4)

Si observamos el Monitor Serie durante la simulación, el flujo de eventos ocurre de la siguiente manera:

1. **Estado de Espera (`Ticks: 186` a `744`):**
   * Al iniciar, se imprime `"RTOS Ready"`. Los vehículos tienen luz verde.
   * La tarea de monitoreo (`monitorTask`, prioridad 1) imprime el estado cada **3 segundos** (~186 ticks): `State: CAR_GREEN | Crossings: 0`.
   * La tarea del controlador (`controllerTask`, prioridad 2) está bloqueada en espera de recibir la señal del botón.

2. **Pulsación del Botón e Interrupción (`[Crossing Requested]`):**
   * Al presionar el botón en la simulación, se dispara la interrupción externa (`buttonISR()`).
   * La ISR entrega el semáforo `xSemButton` (`xSemaphoreGiveFromISR`) y llama a `taskYIELD()` para forzar un cambio de contexto inmediato a la tarea de prioridad alta (`controllerTask`).
   * `controllerTask` se activa instantáneamente, imprime `[Crossing Requested]` e inicia la secuencia del semáforo.

3. **Ciclo del Semáforo (`Ticks: 930` a `2604`):**
   * **`CAR_YELLOW`**: Los vehículos cambian a amarillo por 4 segundos. El monitor registra este estado en los ticks 930 y 1116.
   * **`PED_CROSSING`**: Los autos se detienen en rojo y el semáforo peatonal cambia a verde por 7 segundos. Se incrementa el contador (`Crossings: 1`).
   * **`PED_WARNING`**: La luz verde peatonal parpadea por advertencia (ticks 1860 a 2232).
   * **`COOLDOWN`**: El peatón vuelve a rojo, los autos recuperan el verde y el sistema entra en enfriamiento por 6 segundos para reanudar el tráfico vehicular.

4. **Retorno al estado normal (`Ticks: 2790` en adelante):**
   * Finalizado el enfriamiento, la tarea controladora vacía cualquier rebote del botón acumulado en el semáforo y regresa a `CAR_GREEN` a la espera de un nuevo cruce.

--

## Conclusiones

* **Facilidad en la ejecución de la Máquina de Estados:** El uso de un RTOS simplifica drásticamente el diseño e implementación de una máquina de estados. En lugar de lidiar con complejas estructuras de banderas y condiciones anidadas dentro de un único bucle secuencial, cada estado y sus transiciones fluyen de manera lineal y natural dentro de su propia tarea dedicada, reduciendo errores lógicos de programación.
* **Coordinación de tiempos sin lógica extra:** La naturaleza multitarea y no bloqueante del RTOS permite realizar tareas periódicas (como el monitoreo y diagnóstico por consola) mientras se atienden eventos asíncronos y temporizaciones extensas de forma paralela en un solo núcleo. Se elimina por completo la necesidad de añadir lógica compleja de cronometraje manual (como el uso de múltiples condiciones con `millis()`), delegando toda la gestión y reparto del tiempo de CPU de forma transparente al planificador del sistema operativo.

---

## Referencias

* D. Lacamera, Embedded Systems Architecture: Design and write software for embedded devices to build safe and connected systems, 2a ed. Birmingham, Reino Unido: Packt Publishing, 2023.

* C. L. Liu y J. W. Layland, "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment," Journal of the ACM, 1973.
* R. Barry y el equipo de FreeRTOS, Mastering the FreeRTOS™ Real Time Kernel: A Hands-On Tutorial Guide, Vers. 1.1.0, 2023.
* Documentación Técnica y Artículos (Basados en la Web)
* FreeRTOS, "RTOS Fundamentals," FreeRTOS™, Jun. 2026. [En línea]. Disponible en: <https://www.freertos.org>
* FreeRTOS, "FreeRTOS Kernel Quick Start Guide," FreeRTOS™, Jun. 2026. [En línea]. Disponible en: <https://www.freertos.org>
* A. Rawat, "FreeRTOS on Arduino (Part 1): Simple Multitasking," ControllersTech, 5 de feb. de 2026. [En línea]. Disponible en el portal de tutoriales de ControllersTech.
* L. Llamas, "Cómo usar FreeRTOS en Arduino," Luis Llamas Ingeniería, informática y diseño, 2026. [En línea]. Disponible en el portal de Luis Llamas.
* P. Stevens / feilipu, "Arduino_FreeRTOS_Library: A FreeRTOS Library for all Arduino ATmega Devices (Uno R3, Leonardo, Mega, etc)," GitHub, 29 de sep. de 2024. [En línea]. Disponible en: <https://github.com/feilipu/Arduino_FreeRTOS_Library>
