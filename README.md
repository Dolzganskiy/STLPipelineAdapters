# STL Pipeline Adapters

**STL Pipeline Adapters** — header-only библиотека на **C++23** для декларативной обработки контейнеров, файлов и потоков данных через цепочки адаптеров.

Проект реализует собственную упрощённую модель, похожую по идее на `std::ranges`: источник данных соединяется с преобразованиями оператором `|`, промежуточные операции выполняются поэлементно, а терминальные адаптеры материализуют или выводят результат.

```cpp
auto result = AsDataFlow(values)
    | Filter([](int value) { return value % 2 == 0; })
    | Transform([](int value) { return value * value; })
    | AsVector();
```

## О проекте

Библиотека позволяет строить читаемые конвейеры обработки данных без ручного написания вложенных циклов.

В проекте самостоятельно реализованы:

- единый pull-based интерфейс потока через метод `Next()`;
- композиция источников и адаптеров оператором `|`;
- поддержка range-based `for`;
- ленивые фильтрация и преобразование;
- чтение директорий и файлов;
- разбиение строк и потоков на токены;
- агрегация по ключу;
- аналог `LEFT JOIN`;
- разделение `std::expected` на успешную и ошибочную ветви;
- преобразование результата в `std::vector`;
- вывод элементов в поток.

Использование `std::ranges` в реализации не требуется: основные механизмы потока, итерации и композиции написаны вручную.

## Возможности

| Компонент | Назначение | Выполнение |
|---|---|---|
| `AsDataFlow` | преобразует контейнер в поток ссылок на элементы | ленивое |
| `Dir` | обходит обычную или рекурсивную директорию | ленивое |
| `Filter` | пропускает элементы, удовлетворяющие предикату | ленивое |
| `Transform` | применяет функцию к каждому элементу | ленивое |
| `DropNullopt` | удаляет пустые `std::optional` | ленивое |
| `OpenFiles` | читает содержимое файлов по переданным путям | ленивое между файлами |
| `Split` | разбивает строки или входные потоки по набору разделителей | ленивое |
| `SplitExpected` | разделяет `std::expected` на ошибки и значения | ленивое с буферизацией |
| `AggregateByKey` | группирует элементы по ключу и обновляет аккумулятор | энергичное |
| `Join` | выполняет левое объединение двух потоков | правая часть индексируется заранее |
| `AsVector` | собирает поток в `std::vector` | терминальное |
| `Out` | выводит элементы без разделителя | терминальное |
| `Write` | выводит элементы с заданным разделителем | терминальное |

## Пример: подсчёт частоты слов

```cpp
#include <processing.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: word-counter <directory>\n";
        return 1;
    }

    Dir(argv[1], false)
        | Filter([](const std::filesystem::path& path) {
              return path.extension() == ".txt";
          })
        | OpenFiles()
        | Split("\n\t ,.;:!?")
        | Filter([](const std::string& token) {
              return !token.empty();
          })
        | Transform([](std::string& token) {
              std::transform(
                  token.begin(),
                  token.end(),
                  token.begin(),
                  [](unsigned char ch) {
                      return static_cast<char>(std::tolower(ch));
                  }
              );
              return token;
          })
        | AggregateByKey(
              std::size_t{0},
              [](const std::string&, std::size_t& count) {
                  ++count;
              },
              [](const std::string& token) {
                  return token;
              }
          )
        | Transform([](const auto& statistic) {
              return std::format(
                  "{} — {}",
                  statistic.first,
                  statistic.second
              );
          })
        | Write(std::cout, '\n');
}
```

Пример конвейера:

```text
Директория
    ↓
Фильтрация .txt
    ↓
Чтение файлов
    ↓
Разбиение на слова
    ↓
Нормализация регистра
    ↓
Группировка и подсчёт
    ↓
Форматирование
    ↓
Вывод
```

## Как устроен поток данных

Каждый источник или промежуточный адаптер предоставляет:

```cpp
std::optional<value_type> Next();
```

`Next()` возвращает очередной элемент либо `std::nullopt`, когда поток закончился.

Например, `FilterFlow` запрашивает элементы у предыдущего этапа до тех пор, пока предикат не вернёт `true`:

```text
Filter::Next()
    ├── previous.Next()
    ├── проверка предиката
    ├── неподходящий элемент → запросить следующий
    └── подходящий элемент → вернуть вызывающему коду
```

Благодаря этому большинству промежуточных адаптеров не требуется заранее создавать отдельный контейнер со всеми результатами.

## Композиция оператором `|`

Оператор конвейера принимает поток и адаптер, а затем передаёт поток в вызываемый объект адаптера:

```cpp
template<class Flow, class Adapter>
requires requires(Adapter adapter, Flow flow) {
    adapter(std::move(flow));
}
auto operator|(Flow flow, Adapter adapter);
```

На каждом этапе формируется новый тип потока:

```text
AsDataFlow(...)
    → Flowiterator

| Filter(...)
    → FilterFlow<Flowiterator, Predicate>

| Transform(...)
    → TransformFlow<FilterFlow<...>, Function>

| AsVector()
    → std::vector<Result>
```

Ошибки несовместимости адаптеров обнаруживаются во время компиляции.

## Поддержка range-based `for`

Общий CRTP-класс `FlowRangeMixin` добавляет потоку `begin()` и `end()`:

```cpp
auto flow = AsDataFlow(values)
    | Filter([](int value) {
          return value > 0;
      });

for (const auto& value : flow) {
    std::cout << value << '\n';
}
```

Итератор имеет семантику однопроходного input iterator и получает очередное значение через `Next()`.

## Источники данных

### `AsDataFlow`

Создаёт поток из существующего контейнера:

```cpp
std::vector<int> values{1, 2, 3, 4};

auto flow = AsDataFlow(values);
```

Элементы передаются через `std::reference_wrapper`, поэтому промежуточные функции могут работать с исходными объектами без создания копии на входе.

Контейнер должен существовать дольше построенного потока.

### `Dir`

Создаёт поток путей из директории:

```cpp
auto files = Dir("./data", false);
auto recursive_files = Dir("./data", true);
```

Второй аргумент включает или отключает рекурсивный обход.

Ошибки открытия директории в текущей реализации приводят к созданию пустого потока.

## Промежуточные адаптеры

### `Filter`

Пропускает только элементы, удовлетворяющие предикату:

```cpp
auto even = AsDataFlow(values)
    | Filter([](int value) {
          return value % 2 == 0;
      });
```

### `Transform`

Преобразует тип или значение элемента:

```cpp
auto strings = AsDataFlow(values)
    | Transform([](int value) {
          return std::to_string(value);
      });
```

Возвращаемый тип функции автоматически становится типом элемента нового потока.

### `Split`

Разбивает строки или объекты, совместимые с `std::istream`, по набору символов-разделителей:

```cpp
std::vector<std::string> lines{
    "one,two",
    "three four"
};

auto tokens = AsDataFlow(lines)
    | Split(", ");
```

Соседние разделители создают пустые токены:

```text
"a||b" → "a", "", "b"
```

### `DropNullopt`

Удаляет отсутствующие значения из потока `std::optional<T>`:

```cpp
std::vector<std::optional<int>> values{
    1,
    std::nullopt,
    3
};

auto present = AsDataFlow(values)
    | DropNullopt()
    | AsVector();
```

Результат:

```text
1, 3
```

### `OpenFiles`

Принимает поток путей и выдаёт содержимое каждого успешно открытого файла:

```cpp
auto contents = Dir("./data", false)
    | Filter([](const auto& path) {
          return path.extension() == ".txt";
      })
    | OpenFiles();
```

Неоткрываемые файлы пропускаются.

В текущей реализации содержимое одного файла целиком считывается в `std::string`.

### `SplitExpected`

Разделяет поток `std::expected<T, E>` на два связанных потока:

```cpp
auto parse = [](const std::string& value)
    -> std::expected<int, std::string> {
    try {
        return std::stoi(value);
    } catch (...) {
        return std::unexpected("Invalid number: " + value);
    }
};

auto [errors, numbers] = AsDataFlow(tokens)
    | Transform(parse)
    | SplitExpected();
```

- первый поток содержит ошибки типа `E`;
- второй поток содержит успешные значения типа `T`;
- оба потока используют общее состояние и не выполняют исходный конвейер дважды.

## Агрегация

### `AggregateByKey`

Группирует элементы по ключу и обновляет отдельный аккумулятор для каждой группы:

```cpp
auto statistics = AsDataFlow(words)
    | AggregateByKey(
          std::size_t{0},
          [](const std::string&, std::size_t& count) {
              ++count;
          },
          [](const std::string& word) {
              return word;
          }
      )
    | AsVector();
```

Для каждого нового ключа создаётся копия начального значения. Порядок групп в результате соответствует порядку первого появления ключей во входном потоке.

`AggregateByKey` полностью потребляет входной поток при создании адаптера и хранит агрегированный результат в памяти.

## Объединение потоков

### Объединение `KV`

```cpp
std::vector<KV<int, std::string>> students{
    {1, "Alice"},
    {2, "Bob"}
};

std::vector<KV<int, std::string>> groups{
    {1, "Backend"}
};

auto result = AsDataFlow(students)
    | Join(AsDataFlow(groups))
    | AsVector();
```

Результат имеет тип:

```cpp
JoinResult<std::string, std::string>
```

Для отсутствующего совпадения поле `joined` содержит `std::nullopt`.

### Объединение по функциям выбора ключа

```cpp
auto result = AsDataFlow(students)
    | Join(
          AsDataFlow(groups),
          [](const Student& student) {
              return student.group_id;
          },
          [](const Group& group) {
              return group.id;
          }
      )
    | AsVector();
```

Семантика соответствует `LEFT JOIN`:

- каждый элемент левого потока попадает в результат;
- правая часть представляется как `std::optional`;
- правая сторона заранее индексируется в `std::unordered_map`.

При нескольких правых элементах с одинаковым ключом сохраняется последнее значение.

## Терминальные адаптеры

### `AsVector`

Полностью потребляет поток и возвращает `std::vector`:

```cpp
auto result = flow | AsVector();
```

### `Out`

Выводит элементы без автоматического разделителя:

```cpp
flow | Out(std::cout);
```

### `Write`

Выводит разделитель после каждого элемента:

```cpp
flow | Write(std::cout, '\n');
```

Например:

```cpp
AsDataFlow(values) | Write(std::cout, ", ");
```

выведет:

```text
1, 2, 3, 
```

## Основные инженерные решения

### CRTP вместо общего виртуального интерфейса

`FlowRangeMixin<Derived>` добавляет поддержку итерации без виртуальных функций и выделения объектов в динамической памяти.

### Статическая композиция типов

Каждый адаптер возвращает новый конкретный шаблонный тип. Вызовы могут быть встроены компилятором, а несовместимые операции обнаруживаются на этапе компиляции.

### Ссылочная передача элементов

`AsDataFlow` выдаёт `std::reference_wrapper`, а функция `Unwrap` скрывает различие между обычным значением и ссылочной обёрткой.

### Однопроходная модель

Поток последовательно потребляется вызовами `Next()`. Это позволяет обрабатывать данные без промежуточного контейнера, но уже прочитанные элементы нельзя получить повторно без создания нового потока.

### Частичная материализация

- `Filter`, `Transform`, `Split` и `DropNullopt` обрабатывают элементы по запросу;
- `AggregateByKey` хранит агрегированные группы;
- `Join` хранит индекс правого потока;
- `AsVector` хранит весь итоговый результат.

## Требования

- компилятор с поддержкой **C++23**;
- **CMake 3.20+**;
- стандартная библиотека C++.

Для тестов используется **GoogleTest 1.16**.

## Сборка

```bash
git clone <repository-url>
cd STLPipelineAdapters

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Запуск демонстрационного приложения:

```bash
./build/bin/labwork8 <directory>
```

Приложение обрабатывает `.txt`-файлы непосредственно в указанной директории. Класс `Dir` также поддерживает рекурсивный режим, но в демонстрации он отключён.

## Запуск тестов

```bash
ctest --test-dir build --output-on-failure
```

При первой конфигурации CMake загружает GoogleTest через `FetchContent`, поэтому требуется доступ к интернету.

В репозитории находится **33 активных теста**, проверяющих:

- преобразование контейнера в поток;
- использование потока в range-based алгоритмах;
- фильтрацию;
- изменение значений и типов;
- разбиение строк и `std::stringstream`;
- сохранение пустых токенов;
- удаление `std::nullopt`;
- подсчёт и сложную агрегацию по ключу;
- оба варианта `Join`;
- чтение файлов;
- разделение `std::expected`;
- вывод с разделителем и без него.

## Что демонстрирует проект

Проект показывает практическое применение следующих возможностей современного C++:

- шаблоны классов и функций;
- variadic-friendly статическая композиция типов;
- concepts и `requires`;
- CRTP;
- собственные итераторы;
- `std::optional`;
- `std::expected`;
- `std::reference_wrapper`;
- perfect forwarding и move semantics;
- `std::invoke_result_t`;
- type traits;
- `std::filesystem`;
- `std::unordered_map` и `std::map`;
- лямбда-выражения;
- написание header-only библиотек;
- unit-тестирование с GoogleTest и GoogleMock.
