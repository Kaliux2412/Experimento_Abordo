# Guía educativa — Por qué el PCB de IGNIS-1 se diseñó así

> Material didáctico para el equipo. No explica *qué* tiene la placa, sino **por qué** cada
> decisión está tomada de esa forma. Si entiendes el porqué, puedes diseñar el siguiente
> tú mismo en lugar de copiar.
>
> Cubre: capacitores de desacoplo, mapeo de capacitores por placa, plano de tierra (GND
> pour) en la placa de radio, geometría de la traza de antena, *stitching vias*, buses de
> comunicación y verificación de pinout contra el datasheet.
> Proyecto: IGNIS-1 (3 placas apiladas — Cerebro / Sensores / RF). KiCad 10.

---

## 1. Capacitores de desacoplo: el concepto antes que las tablas

### El problema
Un chip digital (el ESP32, el LoRa, el GPS) no consume corriente de forma suave. Consume
**a tirones**: cada vez que sus transistores conmutan, pide un pulso de corriente en
nanosegundos. El riel de alimentación (+3V3, +5V) llega por una pista de cobre, y **toda
pista tiene inductancia**. La inductancia se opone a los cambios bruscos de corriente
(`V = L · di/dt`): cuando el chip pide corriente de golpe, el voltaje en su pin **cae**
(se hunde) por un instante. Si cae demasiado, el chip se resetea, corrompe datos o se
porta de forma errática e irreproducible — los bugs más difíciles de cazar.

### La solución
Un capacitor colocado **pegado al pin de alimentación** del chip actúa como un
**tanque local de carga**. Cuando el chip pide su tirón de corriente, lo toma del
capacitor (que está a milímetros) en vez de tirar del regulador (que está a centímetros,
detrás de toda esa inductancia). El capacitor se rellena lento desde el riel entre tirón
y tirón.

> **Analogía:** el regulador es la toma de agua de la casa; el capacitor es un tinaco
> junto a la regadera. Cuando abres la llave de golpe, el tinaco entrega el chorro al
> instante y se rellena solo, despacio. Sin tinaco, sentirías la caída de presión cada vez.

### La regla de oro: **lo más pegado posible**
La efectividad de un capacitor de desacoplo depende de lo corta que sea la espira de
cobre entre el capacitor y el pin del chip. Cuanto más lejos, más inductancia en el
camino, y el capacitor "llega tarde" al tirón. Por eso en el ruteo **el cap de 100nF va
literalmente tocando el pin VCC del chip**, antes que cualquier otra cosa. Es la regla #1
del layout de señal mixta.

### Por qué dos tamaños distintos (100nF + 100µF)
Ningún capacitor es ideal a todas las frecuencias.
- **100nF cerámico (MLCC):** chico, baja inductancia interna, responde a los tirones
  **rápidos** (alta frecuencia). Es el que va pegadísimo al pin.
- **100µF / 220µF (bulk):** grande, mucha carga almacenada, responde a las demandas
  **grandes y lentas** (un pico de transmisión, el arranque del heater). Tiene más
  inductancia, así que no necesita estar tan pegado — basta con que esté en la misma
  zona, alimentando esa carga.

Trabajan en equipo: el bulk cubre el evento grande, el cerámico cubre el filo rápido.

---

## 2. Mapeo de capacitores por placa (y el porqué de cada uno)

Regla general aplicada en las 3 placas: **cada cap vive junto a la carga que sirve, en su
propia placa.** Un cap de desacoplo cruzando el conector entre placas es inútil — la
inductancia del backplane mata su efecto. Por eso, por ejemplo, el bulk del LoRa vive en
la placa RF, no en Cerebro.

### Placa CEREBRO (ESP32 + convertidor DC-DC 3V3)
| Cap | Valor | Sirve a | Por qué ahí |
|---|---|---|---|
| C1 | 22µF | **Entrada** del DC-DC (VIN, lado switch) | Estabiliza la entrada del convertidor; le da carga para conmutar sin ensuciar el riel de batería. |
| C2 | 100µF | **Salida** del DC-DC (VOUT) | Alisa el rizado de conmutación del buck-boost; deja el +3V3 limpio. |
| C10 | 100µF | Bulk del riel 3V3 → pin 3V3 del ESP32 | Tanque grande para los picos de WiFi/CPU del ESP32. |
| C11 | 100nF | Desacoplo → pin 3V3 del ESP32, pegadísimo | Cubre los tirones rápidos del ESP32. Regla #1: lo más cerca posible. |

> Un convertidor DC-DC siempre quiere cap **a la entrada y a la salida**: la entrada
> evita que sus pulsos contaminen lo que tiene detrás; la salida entrega el riel limpio.

### Placa RF (LoRa RFM95 + microSD)
| Cap | Valor | Sirve a | Por qué ahí |
|---|---|---|---|
| C32 | 100µF | Bulk para picos de transmisión → LoRa | Transmitir un paquete LoRa es un **pico de corriente** fuerte y breve. Si el riel se hunde durante el TX, el chip falla. El bulk local lo absorbe. **Vive en la placa RF**, no cruza el backplane. |
| C30 | 100nF | Desacoplo → pin VCC del LoRa | Tirones rápidos del SX1276. Pegado al pin. |
| C31 | 100nF | Desacoplo → pin VDD del microSD | La microSD también pide tirones al escribir. |

### Placa SENSORES (boost 5V + sensores)
| Cap | Valor | Sirve a | Por qué ahí |
|---|---|---|---|
| C4 | 220µF | Bulk del riel 5V → salida del boost / cerca del MQ-135 | El **heater del MQ-135 consume ~150mA constantes** y a tirones al arrancar. Bulk grande junto a esa carga. |
| C20 | 100nF | **NO es desacoplo** — filtro de ruido en la salida analógica del MQ-135 | ⚠️ Punto fino: este cap **no** está en un riel de alimentación. Está en la net `MQ_AOUT` (señal analógica), formando un **filtro RC** con el divisor de resistencias antes de entrar al ADC del ESP32. Limpia el ruido de la lectura. Por eso va **junto al divisor R22/R23**, no en VCC. |

> **Los módulos BME280, MPU6050 y GPS no llevan capacitor discreto**: son módulos de
> breakout que **ya traen el suyo en su propia placita**. Ponerles otro sería redundante.
> Saber qué trae ya cada módulo evita poblar la placa de cobre innecesario.

---

## 3. Plano de tierra (GND pour) en la placa de RF

En la placa RF se **inunda casi toda la placa con cobre conectado a GND, en las dos
capas** (cara superior F.Cu y cara inferior B.Cu). Esto se llama *GND pour* o *vertido de
tierra*. No es decoración: es una decisión de ingeniería de RF.

### Por qué un plano de tierra grande
1. **Camino de retorno corto.** Toda corriente que sale de un chip tiene que volver.
   En RF, la corriente de retorno viaja **justo debajo** de la señal por el plano de
   tierra. Un plano continuo le da el camino más corto posible → menos área de espira →
   **menos antena parásita, menos EMI** (interferencia que ensucia tus propias señales y
   contamina la radio).
2. **Referencia sólida.** Todas las señales miden su voltaje **respecto a GND**. Si GND
   está fragmentado, distintas partes del circuito "ven" tierras ligeramente distintas y
   aparece ruido. Un plano sólido = una sola referencia limpia.
3. **Disipación de calor** y rigidez mecánica, de regalo.

### Por qué el pour va **al final**, después de rutear
El relleno se acomoda **alrededor** de las pistas ya existentes, respetando la distancia
de seguridad (*clearance*). No bloquea el ruteo: ruteas todo normal (incluidas las señales
de la microSD por la cara inferior) y luego el pour rellena los huecos. Si lo pusieras
primero, estorbaría. Orden correcto: **rutear → vertir → rellenar (tecla `B`) → DRC.**

---

## 4. Geometría de la traza de antena

La pista que lleva la señal de radio del chip LoRa (pin ANT) al conector SMA es
**especial**. A 433 MHz ya no es "un cable que conecta dos puntos": es una **línea de
transmisión** y su geometría importa.

### Concepto: impedancia característica (50Ω)
En RF, el chip, la traza, el conector y la antena deben tener todos la **misma impedancia,
50Ω por convención**. Si la traza no es de 50Ω, parte de la potencia de RF **rebota** hacia
atrás en vez de salir por la antena (como un eco en una tubería de diámetro cambiante).
Eso reduce alcance y puede dañar el amplificador del chip. El objetivo del diseño de la
traza es **acercarse a 50Ω**.

### Por qué la traza se ensanchó de 0.25mm a 2mm
La impedancia de una pista depende de su ancho, del grosor del aislante (FR4) y del plano
de tierra debajo. En este FR4 de 1.6mm a dos capas:
- Una microstrip de **50Ω puros pediría ~2.9mm** de ancho — impráctico, ocuparía media placa.
- **0.25mm** (el ancho por defecto) daba una impedancia altísima, muy lejos de 50Ω.
- **2mm** da ~60Ω: no es perfecto, pero está **mucho más cerca** de 50Ω y es realista. Se
  dejó en 2mm como compromiso.

### Por qué no es tan crítico como suena (a 433 MHz)
La longitud de onda a 433 MHz dentro del FR4 es ≈ 360mm. La traza mide ~11.5mm, o sea
**~λ/30**. Una traza **eléctricamente corta** (mucho menor que la longitud de onda)
refleja poco aunque la impedancia no sea perfecta. Por eso a 433 MHz uno se puede relajar;
a 2.4 GHz (WiFi/BLE), donde λ es ~6× más corta, el mismo error sería grave. **La
frecuencia decide cuánto rigor necesitas.**

### CPWG: el truco práctico de 2 capas
Como 50Ω puros pedían 2.9mm, se usó una **coplanar waveguide con tierra (CPWG)**: la traza
de antena lleva **cobre de GND a ambos lados** (en la misma cara) **y el plano de GND
debajo**. Esa tierra rodeando la señal baja la impedancia efectiva y la controla mejor,
permitiendo una traza más estrecha que la microstrip pura. Es la forma estándar de hacer
líneas de RF decentes en placas baratas de 2 capas.

### Regla: nada cruza por debajo de la antena
Debajo de **toda** la traza de antena debe haber **GND continuo y sólido**, sin ninguna
otra señal cruzando por la cara inferior. Si una pista de la microSD pasara justo debajo,
"rompería" el plano de retorno de la antena → la corriente de retorno tendría que rodear
el hueco → espira grande → radiación y pérdidas. **El plano bajo la antena es sagrado.**

---

## 5. Stitching vias (vías de cosido)

Una *via* es un agujero metalizado que conecta cobre de la cara superior con la inferior.
Las *stitching vias* son vías de GND repartidas para "**coser**" los dos planos de tierra.

### Por qué cosen
Tienes pour de GND arriba **y** abajo. Si solo se tocaran por los pads de los componentes,
serían prácticamente **dos planos separados** a frecuencias de RF, y eso reintroduce todos
los problemas que el plano venía a resolver. Las vías cada cierta distancia los **amarran
en un solo plano de tierra tridimensional**.

### Dónde van y por qué
- **A ambos lados de la traza de antena, en pares (~cada 5mm):** atan el GND coplanar de
  arriba con el plano de abajo justo donde la señal de RF lo necesita. Confinan los campos
  y aterrizan el retorno de la antena. Es lo que vuelve "real" a la CPWG.
- **En los 4 pads de GND del conector SMA:** cada uno con su vía, para que el shield del
  conector aterrice directo al plano.
- **Repartidas por toda la placa (~cada 10–15mm):** cosido general de los dos pours.

> Regla práctica: cuanto más alta la frecuencia, más juntas las vías. La separación debe
> ser una fracción pequeña de λ para que el plano se comporte como continuo.

---

## 6. Errores comunes al inundar de cobre (checklist)

- **Islas de cobre aisladas.** Si una pista parte el pour y deja un trozo de cobre **sin
  conexión a GND**, ese trozo flota y actúa de **antena parásita** — peor que no tener
  nada ahí. KiCad las marca; hay que conectarlas con una vía a GND o borrarlas.
- **Vías de stitching sin net.** Cada vía debe tener **net = GND**, no "sin net". Una vía
  sin net no cose nada.
- **Rellenar y correr DRC al final.** Tras vertir, tecla `B` para rellenar las zonas, y
  luego **DRC** (Design Rules Check): debe dar **0 errores** y GND sin nets sueltas.
- **No fragmentar el plano bajo señales sensibles** (antena, relojes), como ya se dijo.

---

## 7. Buses de comunicación: cómo se hablan los chips

El ESP32 (el cerebro) tiene que intercambiar datos con sensores, radio, almacenamiento y
GPS. No usa un cable distinto para cada bit: usa **buses**, protocolos estándar donde
varios dispositivos comparten unas pocas líneas. IGNIS-1 usa cuatro, cada uno elegido
según cuánto dato mueve y qué tan rápido.

### Resumen de los 4 buses de IGNIS-1
| Bus | Líneas (GPIO) | Quién va ahí | Por qué este bus |
|---|---|---|---|
| **I²C** | SDA 33, SCL 32 | BME280, MPU6050 | Sensores: poco dato, lento, ahorra pines. |
| **SPI** | SCK 18, MOSI 23, MISO 19, + CS | LoRa, microSD | Mucho dato, rápido (MHz). |
| **UART2** | RX 16, TX 17 | GPS NEO-6M | El GPS escupe texto NMEA en serie continuo. |
| **ADC** | GPIO 35 | MQ-135 | No es bus digital: lee un **voltaje analógico**. |

### I²C — el bus de los sensores (2 cables)
- **SDA** = *Serial Data* (datos, bidireccional, una sola línea).
- **SCL** = *Serial Clock* (reloj, lo marca el maestro / ESP32).

La gracia del I²C: con **solo 2 cables** conecta muchos dispositivos. No hay un cable de
selección por chip; en su lugar **cada dispositivo tiene una dirección única** (el BME280
es 0x76, el MPU6050 es 0x68). Cuando el ESP32 quiere hablar con uno, manda primero su
dirección por la línea SDA; solo el chip con esa dirección responde, los demás se quedan
callados. Por eso ambos sensores comparten los mismos 2 cables sin chocar: `0x76 ≠ 0x68`.

> **Detalle físico — pull-ups.** I²C es *open-drain*: los chips solo pueden **jalar la
> línea a 0**, nunca empujarla a 1. Quien la regresa a 1 es una **resistencia pull-up**
> (4.7k a +3V3, R20/R21). Sin pull-ups el bus no conmuta y no funciona. Los módulos de
> breakout ya traen las suyas, así que al integrar todo en una placa se deja **un solo
> par** de pull-ups para todo el bus.

### SPI — el bus de radio y almacenamiento (4 líneas, rápido)
Cuando hay que mover **mucho** dato rápido (paquetes de radio, archivos en la microSD),
I²C se queda corto. SPI es más veloz (MHz) a cambio de usar más cables:

| Línea | Significado | Dirección |
|---|---|---|
| **SCK** | *Serial Clock* | maestro marca el ritmo; cada pulso = 1 bit |
| **MOSI** | *Master Out, Slave In* | dato ESP32 → LoRa/microSD |
| **MISO** | *Master In, Slave Out* | dato LoRa/microSD → ESP32 |
| **CS / SS** | *Chip Select* | elige a quién le hablas (NSS_LORA=5, CS_SD=13) |

Los nombres MOSI/MISO **ya dicen la dirección del dato**. SPI es **full-duplex**: manda y
recibe **al mismo tiempo** (por eso dos líneas de datos separadas); en cada pulso de reloj
sale un bit por MOSI y entra otro por MISO, simultáneo.

**Cómo conviven LoRa + microSD en el mismo bus:** SCK, MOSI y MISO son **compartidos** (los
dos esclavos en paralelo). Lo que evita el choque es el **CS dedicado**: el ESP32 baja el
CS del chip con quien quiere hablar; el otro esclavo pone su MISO en **alta impedancia**
(*tri-state*, se desconecta eléctricamente) y no estorba. **Solo un esclavo activo a la
vez.** Por eso en el backplane SCK/MOSI/MISO van una sola vez, pero NSS_LORA y CS_SD van
por separado.

### I²C vs SPI — cuándo cada uno
| | I²C | SPI |
|---|---|---|
| Cables | **2** | 4 (+1 CS por esclavo) |
| Cómo elige esclavo | por **dirección** en el mensaje | por **cable CS** físico |
| Velocidad | lenta (100–400 kHz) | rápida (varios MHz) |
| Dúplex | medio (1 línea de datos) | completo (2 líneas) |
| Uso en IGNIS-1 | sensores (poco dato) | radio + SD (mucho dato) |

Regla mental: **poco dato y ahorrar pines → I²C; mucho dato y velocidad → SPI.**

### UART y ADC — los otros dos
- **UART2** (GPS): comunicación serie simple punto a punto, **2 líneas cruzadas** (el TX de
  uno va al RX del otro). El GPS manda sentencias de texto NMEA a 9600 baud sin que nadie
  se lo pida. No hay maestro/esclavo ni direcciones: solo dos que se hablan.
- **ADC** (MQ-135): no es un bus digital. El sensor de gas entrega un **voltaje analógico**
  proporcional a la concentración, y el ESP32 lo mide con su convertidor analógico-digital.
  (Aquí entra el divisor de resistencias + el filtro C20 de la Sección 2: bajan ese voltaje
  de 0–5V a un rango seguro y limpio para el ADC.)

---

## 8. Verificar el pinout del símbolo contra el datasheet

Durante el bring-up en protoboard apareció un bug que vale la pena estudiar, porque es de
los que **no se ven en la pantalla** y solo salen cuando conectas el hardware real.

### Qué pasó
El símbolo del ESP32 en la librería (`ESP32_DevKit_40`) tenía la numeración de pines
**corrida por uno** (*off-by-one*) en la fila inferior izquierda: le faltaba un pin `3V3`.
Consecuencia en cadena:

| Pin físico | Lo que decía el símbolo | Lo correcto (Freenove) |
|---|---|---|
| 18 | 5V | **3V3** |
| 19 | 5V | 5V |
| 20 | GND | **5V** |

Y peor: en el esquemático había una **tierra (GND) cableada al pin 20**. Pero el pin 20
físico del DevKit es **5V**, no GND. Si esa placa se hubiera fabricado y poblado tal cual,
al encender habrías **cortocircuitado el riel de 5V contra tierra** a través de esa pista.
Humo casi seguro.

### Cómo se detectó
No lo encontró ninguna herramienta automática — el **ERC daba 0 errores**, porque desde el
punto de vista del software todo era coherente (un pin de poder conectado a una tierra es
"válido"). Lo detectó una persona **comparando el pinout impreso en la placa física real**
contra lo que decía el diseño. Esa es la lección central.

### La regla
> **El símbolo no es la verdad. El datasheet (o la serigrafía del módulo físico) es la
> verdad.** Antes de rutear, verifica pin por pin el símbolo contra la fuente oficial.

- Consigue el **pinout oficial** del fabricante (aquí, el PDF/PNG de Freenove). No confíes
  en un símbolo bajado de internet ni en uno hecho a mano sin revisar.
- Revisa **especialmente los pines de alimentación** (3V3, 5V, GND). Un GPIO mal numerado
  te da un bug de firmware molesto; un **pin de poder mal numerado te quema hardware**.
- Ojo con los *off-by-one*: si un pin está mal, **todos los de esa fila a partir de ahí**
  suelen estar corridos. Cuenta la fila completa, no solo el pin sospechoso.

### Por qué el ERC no lo atrapa
El ERC (Electrical Rules Check) verifica **coherencia interna** del esquemático: que no
haya dos salidas peleando, que los pines de poder tengan su bandera, etc. **No sabe** cómo
es el chip real por dentro — usa la definición del símbolo como su única "verdad". Si el
símbolo está mal, el ERC valida el error con cara de satisfacción. **ERC limpio ≠ diseño
correcto.** Es condición necesaria, no suficiente.

### Dónde se corrige (y el detalle de la caché)
El pinout vive en **dos lugares** que hay que sincronizar:
1. La **librería** de símbolos (`.kicad_sym`) — la definición maestra.
2. Una **copia en caché dentro de cada esquemático** (`.kicad_sch`, bloque `lib_symbols`).
   KiCad guarda una copia local para que el proyecto abra aunque la librería no esté.

Corregir solo la librería **no basta**: el esquemático sigue usando su copia vieja hasta
que hagas *Update Symbols from Library*. Y tras arreglar el esquemático, hay que propagar
al layout con **Update PCB from Schematic (F8)**. Orden: **símbolo → esquemático → PCB.**

---

## 9. Glosario rápido

| Término | Qué es |
|---|---|
| **Desacoplo** | Capacitor que aísla un chip de las caídas de voltaje del riel dándole carga local. |
| **Bulk** | Capacitor grande que cubre demandas de corriente grandes y lentas. |
| **MLCC** | Capacitor cerámico multicapa; chico y rápido, ideal para 100nF de desacoplo. |
| **GND pour / vertido** | Rellenar el cobre sobrante de la placa con tierra. |
| **Plano de retorno** | El camino por donde la corriente vuelve al chip; en RF, justo bajo la señal. |
| **Impedancia característica (Z₀)** | "Resistencia" de una línea de transmisión a la RF; objetivo 50Ω. |
| **Microstrip** | Traza sobre un plano de tierra; línea de transmisión básica. |
| **CPWG** | Coplanar waveguide con tierra: traza con GND a los lados y debajo. |
| **λ (longitud de onda)** | Distancia que recorre la onda en un ciclo; decide qué es "corto" o "largo". |
| **Via** | Agujero metalizado que conecta capas de cobre. |
| **Stitching via** | Via de GND que cose dos planos de tierra en uno. |
| **DRC** | Design Rules Check: el verificador de reglas físicas del PCB. |
| **Bus** | Conjunto de líneas compartidas por varios chips para comunicarse. |
| **I²C** | Bus de 2 cables (SDA/SCL); elige chip por dirección. Para sensores. |
| **SPI** | Bus rápido de 4 líneas (SCK/MOSI/MISO/CS); elige chip por CS. Para radio/SD. |
| **MOSI / MISO** | Líneas de datos SPI: maestro→esclavo / esclavo→maestro. |
| **CS / Chip Select** | Línea que activa al esclavo con quien se habla. |
| **Open-drain** | Salida que solo jala a 0; necesita pull-up para volver a 1 (I²C). |
| **Pull-up** | Resistencia que mantiene una línea en 1 cuando nadie la jala. |
| **Tri-state / alta Z** | Salida "desconectada" eléctricamente; deja libre el bus compartido. |
| **Full-duplex** | Manda y recibe al mismo tiempo (SPI). |
| **UART** | Comunicación serie punto a punto de 2 líneas cruzadas (TX↔RX). Para el GPS. |
| **ADC** | Convertidor analógico-digital: mide un voltaje (MQ-135). |
| **ERC** | Electrical Rules Check: verifica coherencia interna del esquemático. No sabe si el símbolo refleja el chip real. |
| **Off-by-one** | Error de conteo de uno: aquí, numeración de pines corrida que desplaza toda una fila. |
| **Datasheet / pinout** | Documento oficial del fabricante; la única fuente de verdad para pines y valores. |
| **lib_symbols (caché)** | Copia local del símbolo dentro del `.kicad_sch`; hay que actualizarla tras editar la librería. |

---

*IGNIS-1 — equipo Ignitia. Decisiones de ruteo tomadas 2026-06-24/25. Documento para
formación del equipo; revisar contra el PCB actual antes de citar valores exactos.*
