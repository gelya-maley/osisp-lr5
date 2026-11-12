# lab05-network — Геометрическая вычислительная служба (вариант 18)

Лабораторная работа 5 по дисциплине «Операционные среды и системное программирование» (БГУИР).
Раздел В, вариант 18: геометрическая вычислительная служба поверх UDP.

- Номер варианта: 18 (раздел В).
- Назначение: UDP-сервер принимает запросы RECTANGLE (площадь и периметр), CIRCLE (площадь и
  длина окружности) и TRIANGLE (площадь по трём сторонам), проверяет параметры и возвращает
  ответ одной датаграммой; клиент измеряет RTT и ведёт статистику успехов, тайм-аутов и байт.
- Транспортный протокол: UDP (Winsock 2), собственный прикладной протокол, датаграмма не более 1200 байт.
- Окружение: Windows 10/11 x64, MSVC 2022 (C++17), Python 3.10+ для тестов и графиков.
- STUDENT_ID = 71, SEED = 7887, BASE_PORT = 20071.

## Сборка

```
scripts\build.bat
```

Результат: `build\server.exe`, `build\client.exe`.

## Полный цикл прогона

1. Запустить сервер в отдельной консоли:

```
build\server.exe --host 127.0.0.1 --port 20071
```

2. Запустить клиента в интерактивном режиме в другой консоли и выполнить проверки,
   включая ошибки и недоступный сервер:

```
build\client.exe --host 127.0.0.1 --port 20071 --id 2 --timeout 2000
> RECTANGLE a=2 b=3
> CIRCLE r=1
> TRIANGLE a=3 b=4 c=5
> TRIANGLE a=1 b=2 c=10
> RECTANGLE a=-1 b=2
> CIRCLE r=10
> FOO
> quit
```

Запрос на порт без сервера демонстрирует отсутствие ответа и тайм-аут:

```
build\client.exe --host 127.0.0.1 --port 20072 --id 3 --timeout 1000
> RECTANGLE a=2 b=3
```

3. Запустить пакетный прогон клиента (генерация запросов с фиксированным SEED):

```
build\client.exe --host 127.0.0.1 --port 20071 --id 1 --mode batch --requests 100 --seed 7887 --timeout 2000 --log data\client1.log
```

4. Завершить сервер сочетанием Ctrl+C в его консоли (штатное завершение с закрытием сокета
   и WSACleanup).

Параметры запуска: `--host`, `--port`, `--id`, `--timeout` (мс), `--requests`, `--seed`,
`--mode` (interactive или batch), `--log` (файл журнала).

## Примеры запросов и ответов

| Запрос | Ответ |
|---|---|
| `RECTANGLE requestId=1 a=2 b=3` | `OK requestId=1 kind=RECTANGLE area=6.000000 perimeter=10.000000` |
| `CIRCLE requestId=2 r=1` | `OK requestId=2 kind=CIRCLE area=3.141593 circumference=6.283185` |
| `TRIANGLE requestId=3 a=3 b=4 c=5` | `OK requestId=3 kind=TRIANGLE area=6.000000` |
| `TRIANGLE requestId=4 a=1 b=2 c=10` | `ERROR requestId=4 code=IMPOSSIBLE_TRIANGLE` |
| `RECTANGLE requestId=5 a=-1 b=2` | `ERROR requestId=5 code=BAD_DIMENSIONS` |
| `FOO requestId=6` | `ERROR requestId=6 code=UNKNOWN_COMMAND` |

## Тесты

```
cd tests
python smoke_test.py
```

11 проверок протокола, вычислений, кодов ошибок, превышения размера датаграммы,
недоступного сервера и трёх одновременных клиентов.

## Эксперименты

```
python scripts\run_experiments.py
```

Конфигурации: A — 100 запросов / 1 клиент, B — 500 запросов / 1 клиент,
C — 100 запросов / 3 клиента, D — 500 запросов / 3 клиента; по 3 запуска каждой.
Результаты: `data/experiments.csv`, журналы клиентов в `data/`.

Графики и диаграммы:

```
python scripts\plot_rtt.py
python scripts\plot_errors.py
python scripts\make_diagrams.py
python scripts\sync_listings.py
```

PNG сохраняются в `report/img/` (чёрно-белые), синхронные копии листингов — в `report/listings/`.
Редактируемые исходники диаграмм: `diagrams/sequence.puml`, `diagrams/architecture.drawio`.

## Отчёт

Сборка отчёта (Typst, шаблон bsuir_typst по СТП 01-2024):

```
typst compile report\main.typ report\main.pdf
```

Проверка оформления: `python <путь к bsuir_typst>\linter\linter.py report\main.typ`
(перед запуском задать `set "PYTHONUTF8=1"`).
