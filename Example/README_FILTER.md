# Фильтрация данных в TVirtualTreeView (C++20)

## Описание

Реализована функция фильтрации данных для `TVirtualStringTree` с использованием современных возможностей C++20:
- **Ranges** для элегантной фильтрации
- **Регистронезависимый поиск** по всем полям
- **Поиск по всем типам данных**: текстовые, числовые, даты, boolean, blob

## Использование

### Базовый пример

```cpp
// В вашей форме
void __fastcall TfrmMainExample::edtFilterChange(TObject *Sender)
{
    // Получаем текст из поля ввода (конвертируем в UTF-8)
    std::string filterText = UTF8String(edtFilter->Text).c_str();

    // Применяем фильтр
    FilterData(filterText);
}

void __fastcall TfrmMainExample::btnResetFilterClick(TObject *Sender)
{
    // Сбрасываем фильтр
    ResetFilter();
}
```

### Пример с кнопкой "Применить фильтр"

```cpp
// В вашей форме добавьте:
// - TEdit *edtFilter;     // Поле ввода для текста фильтра
// - TButton *btnApply;    // Кнопка "Применить"
// - TButton *btnReset;    // Кнопка "Сбросить"

void __fastcall TfrmMainExample::btnApplyClick(TObject *Sender)
{
    std::string searchText = UTF8String(edtFilter->Text).c_str();
    FilterData(searchText);

    // Показываем количество найденных записей
    if (vsTreeHandler_) {
        size_t count = vsTreeHandler_->GetFilteredCount();
        lblStatus->Caption = UnicodeString::Format(
            L"Найдено записей: %d из %d",
            ARRAYOFCONST((static_cast<int>(count),
                         static_cast<int>(cachedTreeData_.size())))
        );
    }
}

void __fastcall TfrmMainExample::btnResetClick(TObject *Sender)
{
    edtFilter->Text = L"";
    ResetFilter();
    lblStatus->Caption = L"Фильтр снят";
}
```

### Пример с фильтрацией "на лету" (при вводе)

```cpp
// Добавьте событие OnChange для TEdit
void __fastcall TfrmMainExample::edtFilterChange(TObject *Sender)
{
    std::string filterText = UTF8String(edtFilter->Text).c_str();

    // Применяем фильтр сразу при изменении текста
    FilterData(filterText);
}
```

## API

### Публичные методы формы

#### `void FilterData(const std::string& filterText)`

Применяет фильтр к данным в гриде.

**Параметры:**
- `filterText` - текст для поиска в UTF-8 (пустая строка сбрасывает фильтр)

**Особенности:**
- Поиск регистронезависимый
- Кодировка UTF-8 для поддержки Unicode
- Поиск выполняется по всем полям:
  - Текстовые поля: `fVarchar`, `fChar`, `fBlobT`
  - Числовые поля: `id`, `fBigint`, `fInteger`, `fSmalint`, `fFloat`, `fDoublePrecision`
  - Boolean: `fBoolean` (поиск по "true"/"false")
  - Даты и время: `fDate`, `fTime`, `fTimeshtamp` (поиск по отформатированным значениям)

**Пример:**
```cpp
// Найти все записи содержащие "test"
FilterData("test");

// Найти все записи с ID содержащим "123"
FilterData("123");

// Найти булевы значения
FilterData("true");

// Сбросить фильтр
FilterData("");

// Пример с преобразованием из TEdit
std::string text = UTF8String(edtFilter->Text).c_str();
FilterData(text);
```

#### `void ResetFilter()`

Сбрасывает фильтр и показывает все данные.

**Пример:**
```cpp
ResetFilter();
```

### Методы TTableTest1TreeHandler

#### `void ApplyFilter(const std::string& filterText)`

Внутренний метод фильтрации (используется через `FilterData`).

**Параметры:**
- `filterText` - текст для поиска в UTF-8

#### `void ResetFilter()`

Внутренний метод сброса фильтра (используется через форму).

#### `size_t GetFilteredCount() const`

Возвращает количество отфильтрованных строк.

**Пример:**
```cpp
if (vsTreeHandler_) {
    size_t count = vsTreeHandler_->GetFilteredCount();
    ShowMessage(UnicodeString::Format(L"Найдено: %d", ARRAYOFCONST((static_cast<int>(count)))));
}
```

## Технические детали

### C++20 Ranges

Фильтрация реализована с использованием C++20 ranges:

```cpp
// Создаём view индексов [0, 1, 2, ... cache_->size()-1]
auto indices = views::iota(size_t{0}, cache_->size());

// Фильтруем индексы по условию
auto filtered = indices
    | views::filter([this, &filterLower](size_t idx) {
        return MatchesFilter((*cache_)[idx], filterLower);
    });

// Копируем отфильтрованные индексы в вектор
rngs::copy(filtered, std::back_inserter(filteredIndices_));
```

### Архитектура

Фильтрация следует Model-View паттерну:
- **Модель**: `std::vector<TABLE_TEST_1_SOut> cachedTreeData_` - все данные
- **Фильтр**: `std::vector<size_t> filteredIndices_` - индексы отфильтрованных строк
- **View**: `TVirtualStringTree` - отображение

### Производительность

- **O(n)** - фильтрация по всем записям
- **O(1)** - доступ к отфильтрованной строке
- Используется **ленивое вычисление** (lazy evaluation) через ranges
- Нет копирования данных - только индексы

## Требования

- **C++ Builder 12.0** или новее (RAD Studio 12.0)
- **C++20** включен в настройках проекта:
  - Project → Options → C++ Compiler → Language
  - C++ Language Standard → C++20 (или `/std:c++20`)

## Примеры поиска

| Поисковый запрос | Что будет найдено |
|------------------|-------------------|
| `test` | Все записи где любое текстовое поле содержит "test" (регистронезависимо) |
| `123` | Записи с ID=123, или любым числовым полем содержащим 123 |
| `true` | Записи где `fBoolean = true` |
| `2024` | Записи содержащие 2024 в любом поле (числа, даты) |
| `john` | Записи где varchar/char/blob содержит "john" |
| `` (пустая строка) | Сброс фильтра, показать все |

## Расширение функционала

### Добавление фильтрации по конкретному полю

Если нужно фильтровать только по определенному полю, можно добавить метод:

```cpp
// В TTableTest1TreeHandler
void ApplyFilterByField(const std::wstring& filterText, int fieldIndex) {
    // Реализация фильтрации по конкретному полю
}
```

### Комбинированные фильтры

Для реализации сложных фильтров (AND/OR условия):

```cpp
// В TTableTest1TreeHandler
void ApplyComplexFilter(
    const std::string& textFilter,
    std::optional<int> minId,
    std::optional<int> maxId
) {
    std::string filterLower = ToLowerString(textFilter);

    auto filtered = views::iota(size_t{0}, cache_->size())
        | views::filter([&](size_t idx) {
            const auto& row = (*cache_)[idx];

            // Проверка текста
            if (!textFilter.empty() && !MatchesFilter(row, filterLower)) {
                return false;
            }

            // Проверка диапазона ID
            if (minId && row.id < *minId) return false;
            if (maxId && row.id > *maxId) return false;

            return true;
        });

    rngs::copy(filtered, std::back_inserter(filteredIndices_));
}
```

## Отладка

Для отладки фильтрации можно добавить логирование:

```cpp
void TTableTest1TreeHandler::ApplyFilter(const std::string& filterText) {
    // ... код фильтрации ...

    // Логирование результатов
    OutputDebugString(
        UnicodeString::Format(
            L"Filter applied: '%s', found %d of %d records",
            ARRAYOFCONST((UnicodeString(UTF8String(filterText.c_str())),
                         static_cast<int>(filteredIndices_.size()),
                         static_cast<int>(cache_->size())))
        ).c_str()
    );
}
```

## Работа с UTF-8

Все функции фильтрации используют `std::string` с кодировкой UTF-8:

```cpp
// Преобразование из UnicodeString (TEdit) в std::string (UTF-8)
std::string filterText = UTF8String(edtFilter->Text).c_str();

// Преобразование обратно в UnicodeString (для отображения)
UnicodeString displayText = UTF8String(filterText.c_str());
```

**Преимущества UTF-8:**
- Совместимость с стандартными C++ строковыми функциями
- Меньше памяти для ASCII символов
- Поддержка всех Unicode символов
- Стандарт для современного C++
