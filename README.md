# 📘 Система дистанційного спостереження за станом здоров'я пацієнта

> *Апаратно-програмний IoT-пристрій на базі ESP-WROOM-32 для вимірювання температури тіла та частоти серцевих скорочень з передачею даних до Google Sheets через Wi-Fi.*

---

## 👤 Автор

- **ПІБ**: Туранська Надія
- **Група**: ФЕС-41
- **Керівник**: Кушнір Олексій, доцент
- **Дата виконання**: 26.05.2026

---

## 📌 Загальна інформація

- **Тип проєкту**: Мікроконтролерна вбудована IoT-система на базі ESP32
- **Мова програмування**: C++ (Arduino)
- **Серверна частина**: Google Apps Script (JavaScript)
- **Мікроконтролер**: ESP-WROOM-32 (ESP32)
- **Середовище розробки**: Arduino IDE 2.x
- **Хмарне сховище**: Google Sheets

---

## 🧠 Опис функціоналу

- 🌡️ Вимірювання температури тіла за допомогою датчика DS18B20
- 💓 Вимірювання частоти серцевих скорочень за допомогою датчика PulseSensor SEN-11574
- 🖥️ Відображення показників та стану системи на кольоровому OLED-дисплеї SSD1331
- 🕐 Фіксація точного часу кожного вимірювання через модуль DS3231 (RTC)
- 💾 Локальне збереження даних на MicroSD-карту у форматі CSV (автономний режим без Wi-Fi)
- ☁️ Передача даних до Google Sheets через HTTP GET-запити та Google Apps Script
- 🔄 Автоматична синхронізація накопичених даних із SD-карти після відновлення Wi-Fi
- 🔔 Звукове підтвердження дій через buzzer
- 🔘 Три кнопки керування: **START** (запуск), **CANCEL** (скасування), **SEND** (передача)
- 🔋 Автономне живлення від акумулятора 18650

---

## 🧱 Опис основних файлів

| Файл | Призначення |
|------|-------------|
| `ESP32_Monitoring_System.ino` | Основна прошивка ESP32: ініціалізація, вимірювання, збереження, передача даних |
| `Code Google Apps Script` | Google Apps Script: прийом HTTP-запитів від ESP32 та запис у Google Sheets |
| `/data.txt` (SD-карта) | Локальний CSV-файл для збереження вимірювань при відсутності Wi-Fi |

---

## ▶️ Як запустити проєкт "з нуля"

### 1. Встановлення інструментів

- Arduino IDE v2.x ([завантажити](https://www.arduino.cc/en/software))
- Обліковий запис Google (для Google Sheets та Apps Script)

### 2. Додавання підтримки ESP32 в Arduino IDE

```
File → Preferences → Additional boards manager URLs:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

Потім: `Tools → Board → Boards Manager` → знайти `esp32 by Espressif Systems` → встановити.

### 3. Встановлення бібліотек

У `Tools → Manage Libraries` встановити:

```
OneWire
DallasTemperature
RTClib by Adafruit
Adafruit GFX Library
Adafruit SSD1331 OLED Driver Library
SD (вбудована)
WiFi (вбудована для ESP32)
HTTPClient (вбудована для ESP32)
```

### 4. Налаштування Google Sheets та Apps Script

1. Створіть нову Google Таблицю
2. Перейменуйте аркуш відповідно до імені пацієнта (наприклад: `Pacient`)
3. Відкрийте `Розширення → Apps Script`, вставте код із `Code Google Apps Script.gs`
4. У скрипті вкажіть ID таблиці у змінній `sheet_id`
5. Натисніть `Розгорнути → Нове розгортання`, тип — «Веб-додаток», доступ — «Усі»
6. Скопіюйте ID розгортання з URL

### 5. Налаштування прошивки

Перед завантаженням відредагуйте змінні у `ESP32_Monitoring_System.ino`:

```cpp
const char* ssid     = "Назва_WiFi_мережі";
const char* password = "Пароль_WiFi";

String GOOGLE_SCRIPT_ID = "ID_розгорнутого_скрипту";
String patient           = "Pacient"; // має збігатися з назвою аркуша в Google Sheets
```

### 6. Завантаження прошивки

1. Підключіть ESP32 до комп'ютера через USB
2. У Arduino IDE оберіть `Tools → Board → ESP32 Dev Module`
3. Оберіть відповідний COM-порт у `Tools → Port`
4. Натисніть **Upload**

---

## 🔌 API — формат HTTP-запиту до Google Apps Script

### Надсилання вимірювання

**GET** `https://script.google.com/macros/s/<SCRIPT_ID>/exec`

```
?pacient=Pacient
&date=2026-05-25T14:32:00
&temperature=36.6
&pulse=72
```

**Відповідь сервера:**

```
OK
```

---

## 📷 Схема підключення компонентів

| Пін ESP32 | Компонент |
|-----------|-----------|
| GPIO 34 | PulseSensor SEN-11574 (аналоговий вхід) |
| GPIO 32 | DS18B20 (OneWire) |
| GPIO 22 | SDA – DS3231 (I²C) |
| GPIO 19 | SCL – DS3231 (I²C) |
| GPIO 5 | CS – OLED SSD1331 (SPI) |
| GPIO 4 | DC – OLED SSD1331 |
| GPIO 15 | RST – OLED SSD1331 |
| GPIO 23 | MOSI – OLED SSD1331 |
| GPIO 18 | SCLK – OLED SSD1331 |
| GPIO 26 | CS – MicroSD (HSPI) |
| GPIO 27 | MISO – MicroSD (HSPI) |
| GPIO 14 | CLK – MicroSD (HSPI) |
| GPIO 13 | MOSI – MicroSD (HSPI) |
| GPIO 25 | Buzzer |
| GPIO 21 | Кнопка START |
| GPIO 35 | Кнопка CANCEL |
| GPIO 33 | Кнопка SEND |

---

## 🖱️ Інструкція для користувача

1. **Увімкнення пристрою** — на дисплеї з'являється `System ready`, система підключається до Wi-Fi

2. **Вимірювання**:
   - Натисніть кнопку `▶️ START` — починається 3-хвилинне вимірювання
   - Докладіть палець до датчика пульсу — на дисплеї відображається `Recording...`
   - Якщо палець не виявлено — відображається `No finger`
   - Після завершення дані автоматично зберігаються на SD-карту

3. **Передача даних до Google Sheets**:
   - Натисніть кнопку `📤 SEND`
   - Система надсилає всі збережені записи до Google Таблиці
   - При успіху — файл на SD-карті очищується, buzzer підтверджує звуком

4. **Скасування**:
   - Кнопка `❌ CANCEL` перериває поточне вимірювання

---

## 🧪 Проблеми і рішення

| Проблема | Рішення |
|----------|---------|
| `SD FAIL` на Serial Monitor | Перевірити підключення SD-карти, переконатися що вона відформатована у FAT32 |
| `RTC not found!` | Перевірити підключення SDA/SCL (GPIO 22 та 19), живлення DS3231 |
| Пульс завжди 0 або некоректний | Переконатися, що палець щільно прикладений до датчика; перевірити поріг `fingerThreshold` |
| Wi-Fi не підключається | Перевірити правильність `ssid` та `password`; ESP32 підтримує лише 2,4 ГГц |
| Google Sheets не отримує дані | Перевірити `GOOGLE_SCRIPT_ID`; переконатись, що Apps Script розгорнуто з доступом «Усі» |
| `No valid pulse. Data not saved` | Датчик не зафіксував коректний пульс — повторити вимірювання |

---

## 🧾 Використані джерела / література

- Документація ESP32 — [docs.espressif.com](https://docs.espressif.com)
- Arduino IDE — [arduino.cc](https://www.arduino.cc)
- Бібліотека DallasTemperature — [github.com/milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- Бібліотека RTClib — [github.com/adafruit/RTClib](https://github.com/adafruit/RTClib)
- Adafruit SSD1331 Library — [github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino](https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino)
- PulseSensor — [pulsesensor.com](https://pulsesensor.com)
- Google Apps Script — [developers.google.com/apps-script](https://developers.google.com/apps-script)
