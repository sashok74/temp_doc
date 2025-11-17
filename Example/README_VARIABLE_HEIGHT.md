# Переменная высота строк в TVirtualStringTree

## Описание

В адаптере реализована поддержка переменной высоты строк через событие `OnMeasureItem`. Высота каждой строки вычисляется автоматически на основе содержимого данных.

## Как это работает

Адаптер автоматически подключает обработчик `OnMeasureItem` при вызове `SetupTreeForTableTest1()`. Вам **не нужно** ничего делать дополнительно - переменная высота работает "из коробки".

## Текущая логика определения высоты

По умолчанию высота строк определяется следующим образом:

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    int& NodeHeight)
{
    const TABLE_TEST_1_SOut& row = (*cache_)[realIndex];

    // 1. Увеличенная высота для строк с текстовым BLOB
    if (row.fBlobT && !row.fBlobT->IsEmpty()) {
        NodeHeight = 60; // Высокая строка
        return;
    }

    // 2. Средняя высота для длинных VARCHAR
    if (row.fVarchar && row.fVarchar->Length() > 50) {
        NodeHeight = 30; // Средняя высота
        return;
    }

    // 3. Выделяем каждую 10-ую строку (визуальные разделители)
    if (row.id % 10 == 0) {
        NodeHeight = 25; // Чуть выше обычной
        return;
    }

    // Высота по умолчанию
    NodeHeight = 18;
}
```

## Настройка логики высоты

Чтобы изменить логику определения высоты, отредактируйте метод `OnMeasureItem` в файле `virtual_tree_adapter.cpp`.

### Пример 1: Фиксированная высота для всех строк

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    int& NodeHeight)
{
    // Все строки одинаковой высоты
    NodeHeight = 25;
}
```

### Пример 2: Высота на основе длины текста

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    int& NodeHeight)
{
    if (!cache_ || cache_->empty()) {
        NodeHeight = 18;
        return;
    }

    NodeData* nodeData = static_cast<NodeData*>(Sender->GetNodeData(Node));
    size_t realIndex = (!filteredIndices_.empty())
        ? filteredIndices_[nodeData->rowIndex]
        : nodeData->rowIndex;

    if (realIndex >= cache_->size()) {
        NodeHeight = 18;
        return;
    }

    const TABLE_TEST_1_SOut& row = (*cache_)[realIndex];

    // Вычисляем высоту на основе длины VARCHAR
    if (row.fVarchar) {
        int textLength = row.fVarchar->Length();

        if (textLength > 100) {
            NodeHeight = 50;
        } else if (textLength > 50) {
            NodeHeight = 30;
        } else {
            NodeHeight = 18;
        }
    } else {
        NodeHeight = 18;
    }
}
```

### Пример 3: Высота на основе типа данных

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    int& NodeHeight)
{
    // ... получение row ...

    // Разная высота для разных типов данных
    if (row.fBlobT && !row.fBlobT->IsEmpty()) {
        NodeHeight = 80; // BLOB - максимальная высота
    } else if (row.fTimeshtamp) {
        NodeHeight = 25; // Временные метки - средняя высота
    } else if (row.fBoolean) {
        NodeHeight = 20; // Boolean - компактная высота
    } else {
        NodeHeight = 18; // Обычные данные
    }
}
```

### Пример 4: Динамическая высота на основе содержимого колонки

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    int& NodeHeight)
{
    // ... получение row ...

    // Вычисляем реальную высоту текста
    if (row.fVarchar) {
        TCanvas* canvas = tree_->Canvas;

        // Ширина колонки VARCHAR (колонка 17)
        int columnWidth = tree_->Header->Columns->Items[17]->Width - 8;

        TRect rect(0, 0, columnWidth, 0);

        // Вычисляем высоту для многострочного текста
        int textHeight = DrawText(
            canvas->Handle,
            row.fVarchar->c_str(),
            row.fVarchar->Length(),
            &rect,
            DT_CALCRECT | DT_WORDBREAK
        );

        // Добавляем отступы
        NodeHeight = textHeight + 8;

        // Минимальная высота
        if (NodeHeight < 18) {
            NodeHeight = 18;
        }
    } else {
        NodeHeight = 18;
    }
}
```

### Пример 5: Чередующиеся высоты (зебра)

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    int& NodeHeight)
{
    // ... получение row ...

    // Чередуем высоты для визуального разделения
    if (row.id % 2 == 0) {
        NodeHeight = 22; // Четные строки выше
    } else {
        NodeHeight = 18; // Нечетные строки обычные
    }
}
```

## Программное изменение высоты конкретной строки

Если нужно изменить высоту конкретной строки программно:

```cpp
// В вашем коде формы
void __fastcall TfrmMainExample::Button2Click(TObject *Sender)
{
    // Получаем первый узел
    PVirtualNode node = vsTree->GetFirst();

    while (node) {
        // Изменяем высоту конкретного узла
        if (node->Index == 5) { // Например, 6-ая строка
            vsTree->NodeHeight[node] = 50; // Устанавливаем высоту
            vsTree->InvalidateNode(node);  // Перерисовываем
        }

        node = vsTree->GetNext(node);
    }
}
```

## Важные замечания

### 1. Опция toVariableNodeHeight

Опция `toVariableNodeHeight` **уже включена** в адаптере автоматически:

```cpp
// В SetupTreeForTableTest1
tree->TreeOptions->MiscOptions = tree->TreeOptions->MiscOptions
    << Virtualtrees::Types::toGridExtensions
    << Virtualtrees::Types::toVariableNodeHeight;  // ← Включено автоматически
```

### 2. Работа с фильтрацией

`OnMeasureItem` **автоматически учитывает фильтрацию**:

```cpp
// Получаем реальный индекс с учетом фильтрации
size_t realIndex = (!filteredIndices_.empty())
    ? filteredIndices_[nodeData->rowIndex]
    : nodeData->rowIndex;
```

Это означает, что при применении фильтра высота строк будет корректно вычисляться для отфильтрованных данных.

### 3. Производительность

`OnMeasureItem` вызывается для каждого узла при:
- Первоначальной загрузке данных
- Скроллинге дерева
- Инвалидации узлов

**Рекомендации:**
- Избегайте сложных вычислений в `OnMeasureItem`
- Кэшируйте результаты, если это возможно
- Используйте простую логику для больших объемов данных

### 4. Минимальная и максимальная высота

Рекомендуется устанавливать разумные границы:

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(...)
{
    // Вычисляем высоту...
    int calculatedHeight = ...;

    // Ограничиваем диапазон
    NodeHeight = std::clamp(calculatedHeight, 18, 200);
}
```

## Отладка

Для отладки высоты строк можно добавить логирование:

```cpp
void __fastcall TTableTest1TreeHandler::OnMeasureItem(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    int& NodeHeight)
{
    // ... вычисление высоты ...

    // Логирование
    OutputDebugString(
        UnicodeString::Format(
            L"Node %d: height = %d",
            ARRAYOFCONST((Node->Index, NodeHeight))
        ).c_str()
    );
}
```

## Примеры использования

### Простой случай: все строки одинаковые

Если вам не нужна переменная высота, просто измените логику в `OnMeasureItem`:

```cpp
NodeHeight = 20; // Все строки высотой 20 пикселей
```

### Средний случай: высота на основе одного поля

```cpp
if (row.fVarchar && row.fVarchar->Length() > 50) {
    NodeHeight = 30;
} else {
    NodeHeight = 18;
}
```

### Сложный случай: высота на основе нескольких полей

```cpp
int maxHeight = 18;

if (row.fVarchar && row.fVarchar->Length() > 50) {
    maxHeight = std::max(maxHeight, 30);
}

if (row.fBlobT && !row.fBlobT->IsEmpty()) {
    maxHeight = std::max(maxHeight, 60);
}

if (row.fTimeshtamp) {
    maxHeight = std::max(maxHeight, 25);
}

NodeHeight = maxHeight;
```

## FAQ

**Q: Могу ли я отключить переменную высоту?**

A: Да, просто установите фиксированную высоту в `OnMeasureItem`:
```cpp
NodeHeight = 18; // Все строки одинаковой высоты
```

**Q: Как изменить высоту всех строк сразу?**

A: Измените логику в `OnMeasureItem` и вызовите:
```cpp
vsTree->Invalidate(); // Перерисует все узлы
```

**Q: Работает ли это с сортировкой?**

A: Да, высота вычисляется на основе данных строки, независимо от порядка.

**Q: Как установить высоту по умолчанию?**

A: Измените последнюю строку в `OnMeasureItem`:
```cpp
NodeHeight = 25; // Вместо 18
```

**Q: Можно ли использовать разную высоту для разных колонок?**

A: Нет, высота устанавливается для всей строки целиком, не для отдельных ячеек.
