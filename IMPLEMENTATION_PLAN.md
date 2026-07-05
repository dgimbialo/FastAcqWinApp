# План реалізації: FastAcq - повне керування Fast Acquisition Device

**Дата:** 04.07.2026
**Зв'язаний проект:** `D:\My_project\ADC_STM32H7\Fast_Acquisition_Device` (прошивка STM32H743; контракт протоколу: `CONTROL_API_PLAN.md` §2 у тому проекті)
**Мета:** додаток повністю керує мікроконтролером: частота/довжина/амплітуда чірпа, серія чірпів (burst),
інтервал, кількість вибірок (32-біт), режими IDLE / CONTINUOUS / SINGLE, отримання RAW і FFT даних,
видимий стан пристрою і журнал усіх команд/відповідей.

---

## 1. Діагноз (аналіз від 04.07.2026)

- Уся інфраструктура керування ВЖЕ написана: `CommandPanel` має кнопки Mode/Data Mask/Interval/Trigger/Status/Freq/Samples/Ping,
  `MainFrameImpl.cpp` має всі обробники `OnCmdSet*`, `SerialWorker::SendCommand` формує коректні 8-байтні команди з CRC8.
- Але в `CommandPanel::OnCreate` ряд 1 (Start/Stop/Freq/Samples/Ping) і весь ряд 2 (Mode/Mask/Interval/Trigger/Status)
  сховані через `ShowWindow(SW_HIDE)` - користувач бачить лише Connect / Save Frame / Clear / PC Mode.
- `OnCmdSetSamples` ріже значення до 16 біт (`arg1`), а буфер MCU тримає 650000 семплів.
- Службові кадри (PONG frame_id=0xFFFFFFFF, STATUS frame_id=0xFFFFFFFE) потрапляють у список чірпів як звичайні кадри.
- Немає команд амплітуди, burst, abort (їх не було в протоколі).

## 2. Оновлення контракту протоколу (ProtocolDefs.h, дзеркало usb_protocol.h)

```cpp
constexpr uint8_t CMD_SET_AMPLITUDE = 0x0A; // arg1 = DAC counts 1..4095
constexpr uint8_t CMD_SET_BURST     = 0x0B; // arg1 = chirps per capture 1..1024
constexpr uint8_t CMD_ABORT         = 0x0C;

constexpr uint8_t FRAME_FLAG_IS_ACK = 1u << 4;

constexpr uint32_t FRAME_ID_PONG   = 0xFFFFFFFFu;
constexpr uint32_t FRAME_ID_STATUS = 0xFFFFFFFEu;
constexpr uint32_t FRAME_ID_ACK    = 0xFFFFFFFDu;
```

- `CMD_SET_SAMPLES`: 32-біт -> `arg1 = low16`, `arg2 = high16`, 0 = auto.
- ACK-кадр: `fft_size` = підтверджений опкод, `fft_peak_bin` = статус (0 OK / 1 BAD_ARG / 2 BAD_STATE / 3 HW_FAIL),
  `actual_samples` = застосоване значення.
- STATUS-кадр: mode у `fft_size`, mask у `fft_peak_bin`, interval у `fft_peak_mag`, samples у `actual_samples`,
  амплітуда у `reserved1[0..1]`, burst у `reserved1[2..3]`, стан FSM у `reserved1[4]`, код помилки у `reserved1[5]`.

## 3. UI (CommandPanel)

1. Прибрати обидва блоки `SW_HIDE` в `OnCreate`.
2. Новий layout, 3 ряди (панель 88 px, оновити `cmdH` у `MainFrameImpl::RelayoutClient`):
   - Ряд 1: COM, Connect, Start, Stop, Trigger, Abort, Ping, Status, Save Frame, Clear
   - Ряд 2: Mode+Apply, Freq+Set, Samples+Set, Interval+Set, Raw/FFT+Data Mask, Amp+Set, Burst+Set
   - Ряд 3: PC Mode (RAW local FFT / FFT from MCU) - як зараз
3. Нові контроли: `IDC_EDT_AMPLITUDE`+`IDC_BTN_SET_AMP`, `IDC_EDT_BURST`+`IDC_BTN_SET_BURST`, `IDC_BTN_ABORT`.
   Дефолти: Amp=4095, Burst=1, Freq=458, Samples=0 (auto), Interval=30.
4. Семантика кнопок:
   - **Start** = `CMD_START_CHIRP(freq)` + `CMD_SET_MODE(CONTINUOUS)` (запустити стрім)
   - **Stop** = `CMD_SET_MODE(IDLE)` (як зараз)
   - **Trigger** = `CMD_TRIGGER` (одиночний чірп у SINGLE/IDLE)
   - **Abort** = `CMD_ABORT` (аварійна зупинка поточного захоплення)

## 4. Службові кадри (SerialWorker + MainFrame)

1. У колбеку парсера в `SerialWorker`: кадри з `frame_id >= FRAME_ID_ACK` або прапорцями IS_STATUS/IS_ACK
   **не** пушити в `ChirpStore`; замість цього `PostMessage(WM_APP_SERVICE_FRAME, type, new ChirpFrame*)`.
2. `CMainFrame::OnServiceFrame`:
   - PONG -> порахувати RTT (перенести логіку з OnFrameReady), рядок у Communication log.
   - STATUS -> декодувати всі поля, показати в статус-барі ("MCU: mode=CONT freq=458Hz amp=4095 burst=1 int=30ms
     samples=auto state=IDLE err=0") і в Communication log.
   - ACK -> рядок у Communication log: "ACK cmd=0x05 status=OK applied=131072" (або NAK з кодом).
3. `WM_APP_SERVICE_FRAME = WM_APP + 37`; нові WM_APP для amplitude/burst/abort з CommandPanel.

## 5. 32-бітні semples

`OnCmdSetSamples`: `SendCommand(CMD_SET_SAMPLES, v & 0xFFFF, (v >> 16) & 0xFFFF)`.
Прибрати обмеження ES_NUMBER на 16 біт немає потреби (поле і так текстове), максимум клампити до 650000.

## 6. Порядок виконання

1. ProtocolDefs.h + AppMessages.h + resource.h (нові опкоди/повідомлення/ID)
2. CommandPanel: розховати, нові контроли, layout
3. SerialWorker: фільтр службових кадрів
4. MainFrame: OnServiceFrame, нові обробники команд, 32-bit samples, cmdH
5. Збірка MSBuild x64 Debug + виправлення

## 7. Відкладено

- Автоматичний GET_STATUS після Connect (poll стану пристрою) - легко додати після перевірки на залізі.
- Retry команд по таймауту ACK.
- Візуалізація burst-серій (розбиття кадру на M чірпів у Waveform).
- Збереження налаштувань між сесіями (registry/ini).
