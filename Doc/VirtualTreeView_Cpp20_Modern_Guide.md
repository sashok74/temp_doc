# TVirtualTreeView: Современные подходы с C++20

## Содержание
1. [Введение](#введение)
2. [Ranges и фильтрация](#ranges-и-фильтрация)
3. [Concepts для типобезопасности](#concepts-для-типобезопасности)
4. [std::span для безопасной работы с данными](#stdspan-для-безопасной-работы-с-данными)
5. [Coroutines для асинхронной загрузки](#coroutines-для-асинхронной-загрузки)
6. [Полный пример с C++20](#полный-пример-с-c20)

---

## Введение

C++20 предоставляет мощные инструменты для работы с данными в TVirtualTreeView:
- **Ranges** - элегантная фильтрация и трансформация данных
- **Concepts** - строгая типизация с понятными сообщениями об ошибках
- **std::span** - безопасная работа с диапазонами без копирования
- **Coroutines** - асинхронная загрузка больших объемов данных

---

## Ranges и фильтрация

### Базовый пример с ranges::views

```cpp
// File: Example/modern_filter_cpp20.h
#ifndef MODERN_FILTER_CPP20_H
#define MODERN_FILTER_CPP20_H

#include <vector>
#include <string>
#include <ranges>
#include <algorithm>
#include "generated/queries/TABLE_TEST_1_SOut.h"

namespace rngs = std::ranges;
namespace views = std::views;

class ModernTableFilter {
private:
    std::vector<generated::queries::TABLE_TEST_1_SOut> allData_;
    std::vector<const generated::queries::TABLE_TEST_1_SOut*> filteredView_;

public:
    // Фильтрация с использованием ranges::views
    void FilterByText(const std::wstring& searchText) {
        filteredView_.clear();

        auto lowerSearch = ToLower(searchText);

        // Используем ranges для элегантной фильтрации
        auto filtered = allData_
            | views::filter([&](const auto& row) {
                return MatchesSearchText(row, lowerSearch);
            })
            | views::transform([](const auto& row) -> const auto* {
                return &row;
            });

        // Копируем результаты в вектор
        rngs::copy(filtered, std::back_inserter(filteredView_));
    }

    // Фильтрация по нескольким условиям с композицией views
    void FilterComplex(const std::wstring& text,
                       std::optional<int> minId,
                       std::optional<int> maxId) {
        filteredView_.clear();

        auto filtered = allData_
            | views::filter([&](const auto& row) {
                // Фильтр по тексту
                if (!text.empty() && !MatchesSearchText(row, ToLower(text))) {
                    return false;
                }
                // Фильтр по минимальному ID
                if (minId && row.id < *minId) {
                    return false;
                }
                // Фильтр по максимальному ID
                if (maxId && row.id > *maxId) {
                    return false;
                }
                return true;
            })
            | views::transform([](const auto& row) { return &row; });

        rngs::copy(filtered, std::back_inserter(filteredView_));
    }

    // Сортировка с ranges::sort
    template<typename Comparator>
    void SortData(Comparator comp) {
        rngs::sort(filteredView_, comp,
                   [](const auto* ptr) -> const auto& { return *ptr; });
    }

    // Получить количество отфильтрованных строк
    size_t GetFilteredCount() const { return filteredView_.size(); }

    // Получить строку по индексу
    const generated::queries::TABLE_TEST_1_SOut& GetRow(size_t index) const {
        return *filteredView_[index];
    }

private:
    static std::wstring ToLower(const std::wstring& str) {
        std::wstring result = str;
        rngs::transform(result, result.begin(), ::towlower);
        return result;
    }

    static bool MatchesSearchText(const generated::queries::TABLE_TEST_1_SOut& row,
                                  const std::wstring& search) {
        if (row.fVarchar) {
            std::wstring value = row.fVarchar.value();
            rngs::transform(value, value.begin(), ::towlower);
            return value.find(search) != std::wstring::npos;
        }
        return false;
    }
};

#endif // MODERN_FILTER_CPP20_H
```

### Пример с ranges::take и ranges::drop для пагинации

```cpp
// Пагинация с использованием ranges
class PaginatedFilter {
private:
    std::vector<generated::queries::TABLE_TEST_1_SOut> allData_;
    size_t pageSize_ = 100;
    size_t currentPage_ = 0;

public:
    // Получить страницу данных
    std::vector<const generated::queries::TABLE_TEST_1_SOut*> GetPage(size_t pageNum) {
        std::vector<const generated::queries::TABLE_TEST_1_SOut*> result;

        auto page = allData_
            | views::drop(pageNum * pageSize_)  // Пропустить предыдущие страницы
            | views::take(pageSize_)             // Взять только текущую страницу
            | views::transform([](const auto& row) { return &row; });

        rngs::copy(page, std::back_inserter(result));
        return result;
    }

    // Получить количество страниц
    size_t GetPageCount() const {
        return (allData_.size() + pageSize_ - 1) / pageSize_;
    }
};
```

---

## Concepts для типобезопасности

### Определение концептов для фильтрации

```cpp
// File: Example/filter_concepts.h
#ifndef FILTER_CONCEPTS_H
#define FILTER_CONCEPTS_H

#include <concepts>
#include <string>
#include <optional>

namespace FilterConcepts {

// Концепт для данных, которые могут быть отфильтрованы
template<typename T>
concept Filterable = requires(T t) {
    { t.id } -> std::convertible_to<int>;
    requires std::same_as<decltype(t.fVarchar), std::optional<std::wstring>>;
};

// Концепт для предиката фильтра
template<typename F, typename T>
concept FilterPredicate = requires(F f, const T& t) {
    { f(t) } -> std::convertible_to<bool>;
};

// Концепт для компаратора сортировки
template<typename C, typename T>
concept Comparator = requires(C c, const T& a, const T& b) {
    { c(a, b) } -> std::convertible_to<bool>;
};

// Концепт для проекции (извлечение поля для сортировки)
template<typename P, typename T>
concept Projection = requires(P p, const T& t) {
    p(t);
};

} // namespace FilterConcepts

// Класс фильтра с использованием концептов
template<FilterConcepts::Filterable RowType>
class TypeSafeFilter {
private:
    std::vector<RowType> allData_;
    std::vector<const RowType*> filteredView_;

public:
    // Метод фильтрации с проверкой типа предиката
    template<FilterConcepts::FilterPredicate<RowType> Pred>
    void Filter(Pred predicate) {
        filteredView_.clear();

        auto filtered = allData_
            | std::views::filter(predicate)
            | std::views::transform([](const auto& row) { return &row; });

        std::ranges::copy(filtered, std::back_inserter(filteredView_));
    }

    // Метод сортировки с проверкой типа компаратора
    template<FilterConcepts::Comparator<RowType> Comp>
    void Sort(Comp comparator) {
        std::ranges::sort(filteredView_, comparator,
                         [](const RowType* ptr) -> const RowType& { return *ptr; });
    }

    // Сортировка с проекцией (по определенному полю)
    template<FilterConcepts::Projection<RowType> Proj>
    void SortBy(Proj projection) {
        std::ranges::sort(filteredView_, std::less{},
                         [&](const RowType* ptr) { return projection(*ptr); });
    }

    size_t Size() const { return filteredView_.size(); }
    const RowType& operator[](size_t idx) const { return *filteredView_[idx]; }
};

// Примеры использования с автоматической проверкой типов
void Example() {
    TypeSafeFilter<generated::queries::TABLE_TEST_1_SOut> filter;

    // Фильтрация - компилятор проверит, что предикат возвращает bool
    filter.Filter([](const auto& row) { return row.id > 100; });

    // Сортировка по ID
    filter.SortBy([](const auto& row) { return row.id; });

    // Сортировка по varchar (с обработкой optional)
    filter.SortBy([](const auto& row) {
        return row.fVarchar.value_or(L"");
    });
}

#endif // FILTER_CONCEPTS_H
```

---

## std::span для безопасной работы с данными

### Использование span для избежания копирования

```cpp
// File: Example/span_adapter.h
#ifndef SPAN_ADAPTER_H
#define SPAN_ADAPTER_H

#include <span>
#include <vector>
#include <VirtualTrees.hpp>

// Адаптер с использованием std::span для безопасного доступа без копирования
class SpanBasedTreeAdapter {
private:
    std::vector<generated::queries::TABLE_TEST_1_SOut> allData_;
    std::vector<size_t> filteredIndices_;

public:
    // Возвращаем span для безопасного доступа к отфильтрованным данным
    std::span<const size_t> GetFilteredIndices() const {
        return std::span(filteredIndices_);
    }

    // Получить span всех данных (read-only)
    std::span<const generated::queries::TABLE_TEST_1_SOut> GetAllData() const {
        return std::span(allData_);
    }

    // Получить span определенного диапазона (для виртуальной прокрутки)
    std::span<const size_t> GetIndicesRange(size_t start, size_t count) const {
        if (start >= filteredIndices_.size()) {
            return {};
        }
        size_t actualCount = std::min(count, filteredIndices_.size() - start);
        return std::span(filteredIndices_).subspan(start, actualCount);
    }

    // OnGetText с использованием span
    void __fastcall OnGetText(TBaseVirtualTree* Sender, PVirtualNode Node,
                               TColumnIndex Column, TVSTTextType TextType,
                               UnicodeString& CellText) {
        auto* nodeData = static_cast<NodeData*>(Sender->GetNodeData(Node));
        size_t nodeIndex = nodeData->rowIndex;

        auto indices = GetFilteredIndices();
        if (nodeIndex >= indices.size()) {
            return;
        }

        auto data = GetAllData();
        size_t dataIndex = indices[nodeIndex];
        const auto& row = data[dataIndex];

        // Форматирование данных...
        switch (Column) {
            case 0: CellText = IntToStr(row.id); break;
            case 1: CellText = row.fVarchar.value_or(L""); break;
        }
    }

    // Обновление данных через span (безопасная модификация диапазона)
    void UpdateRange(std::span<const generated::queries::TABLE_TEST_1_SOut> newData,
                     size_t startIndex) {
        if (startIndex + newData.size() > allData_.size()) {
            throw std::out_of_range("Update range exceeds data size");
        }

        std::copy(newData.begin(), newData.end(),
                  allData_.begin() + startIndex);
    }
};

#endif // SPAN_ADAPTER_H
```

---

## Coroutines для асинхронной загрузки

### Generator для ленивой загрузки данных

```cpp
// File: Example/async_data_loader.h
#ifndef ASYNC_DATA_LOADER_H
#define ASYNC_DATA_LOADER_H

#include <coroutine>
#include <vector>
#include <optional>
#include <memory>

// Простой генератор для ленивой загрузки данных
template<typename T>
class Generator {
public:
    struct promise_type {
        T current_value;

        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) {
            current_value = std::move(value);
            return {};
        }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    struct iterator {
        std::coroutine_handle<promise_type> handle_;
        bool done_;

        iterator(std::coroutine_handle<promise_type> h, bool done)
            : handle_(h), done_(done) {
            if (!done_) {
                ++(*this);
            }
        }

        iterator& operator++() {
            handle_.resume();
            done_ = handle_.done();
            return *this;
        }

        const T& operator*() const {
            return handle_.promise().current_value;
        }

        bool operator==(const iterator& other) const {
            return done_ == other.done_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    iterator begin() {
        return iterator{handle_, false};
    }

    iterator end() {
        return iterator{handle_, true};
    }

    explicit Generator(std::coroutine_handle<promise_type> h) : handle_(h) {}

    ~Generator() {
        if (handle_) handle_.destroy();
    }

    Generator(Generator&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

private:
    std::coroutine_handle<promise_type> handle_;
};

// Класс для асинхронной загрузки и фильтрации данных
class AsyncDataLoader {
public:
    // Генератор для ленивой загрузки данных по частям
    Generator<std::vector<generated::queries::TABLE_TEST_1_SOut>>
    LoadDataBatches(size_t batchSize = 1000) {
        // Симуляция загрузки данных из БД порциями
        for (size_t offset = 0; offset < totalRecords_; offset += batchSize) {
            std::vector<generated::queries::TABLE_TEST_1_SOut> batch;
            batch.reserve(batchSize);

            // Загрузка одной порции
            // В реальности здесь был бы запрос к БД
            for (size_t i = 0; i < batchSize && (offset + i) < totalRecords_; ++i) {
                batch.push_back(LoadRecord(offset + i));
            }

            co_yield batch;
        }
    }

    // Генератор для фильтрации с ленивым вычислением
    Generator<generated::queries::TABLE_TEST_1_SOut>
    FilteredStream(const std::wstring& filterText) {
        for (const auto& record : allData_) {
            if (MatchesFilter(record, filterText)) {
                co_yield record;
            }
        }
    }

private:
    std::vector<generated::queries::TABLE_TEST_1_SOut> allData_;
    size_t totalRecords_ = 0;

    generated::queries::TABLE_TEST_1_SOut LoadRecord(size_t index) {
        // Заглушка - загрузка записи
        generated::queries::TABLE_TEST_1_SOut record;
        record.id = static_cast<int>(index);
        return record;
    }

    bool MatchesFilter(const generated::queries::TABLE_TEST_1_SOut& record,
                      const std::wstring& filter) {
        return record.fVarchar &&
               record.fVarchar->find(filter) != std::wstring::npos;
    }
};

// Пример использования с TVirtualTreeView
class AsyncTreeAdapter {
private:
    AsyncDataLoader loader_;
    std::vector<generated::queries::TABLE_TEST_1_SOut> currentData_;
    TVirtualStringTree* tree_;

public:
    // Загрузка данных порциями с обновлением дерева
    void LoadDataAsync() {
        currentData_.clear();

        // Загружаем данные порциями
        for (auto batch : loader_.LoadDataBatches(1000)) {
            // Добавляем порцию к текущим данным
            currentData_.insert(currentData_.end(),
                              std::make_move_iterator(batch.begin()),
                              std::make_move_iterator(batch.end()));

            // Обновляем дерево после каждой порции
            tree_->RootNodeCount = currentData_.size();
            tree_->Invalidate();

            // Даем интерфейсу обновиться (в реальности используйте TThread)
            Application->ProcessMessages();
        }
    }

    // Фильтрация с ленивым вычислением
    void ApplyFilterLazy(const std::wstring& filterText) {
        currentData_.clear();

        // Получаем отфильтрованные данные по одной записи
        for (auto record : loader_.FilteredStream(filterText)) {
            currentData_.push_back(std::move(record));

            // Обновляем UI каждые N записей
            if (currentData_.size() % 100 == 0) {
                tree_->RootNodeCount = currentData_.size();
                tree_->Invalidate();
                Application->ProcessMessages();
            }
        }

        // Финальное обновление
        tree_->RootNodeCount = currentData_.size();
        tree_->Invalidate();
    }
};

#endif // ASYNC_DATA_LOADER_H
```

---

## Полный пример с C++20

### Современная реализация фильтра с использованием всех возможностей C++20

```cpp
// File: Example/modern_tree_filter_full.h
#ifndef MODERN_TREE_FILTER_FULL_H
#define MODERN_TREE_FILTER_FULL_H

#include <vector>
#include <string>
#include <ranges>
#include <concepts>
#include <span>
#include <functional>
#include <optional>
#include <algorithm>
#include "generated/queries/TABLE_TEST_1_SOut.h"
#include <VirtualTrees.hpp>

namespace rngs = std::ranges;
namespace views = std::views;

// ==================== CONCEPTS ====================

template<typename T>
concept TableRow = requires(T t) {
    { t.id } -> std::convertible_to<int>;
};

template<typename F, typename T>
concept RowPredicate = std::predicate<F, const T&>;

template<typename C, typename T>
concept RowComparator = std::strict_weak_order<C, const T*, const T*>;

// ==================== MODERN FILTER CLASS ====================

template<TableRow RowType>
class ModernTreeFilter {
private:
    std::vector<RowType> allData_;
    std::vector<const RowType*> filteredView_;
    std::function<bool(const RowType&)> currentPredicate_;

    // Компаратор для сортировки
    struct PointerComparator {
        std::function<bool(const RowType&, const RowType&)> comp_;

        bool operator()(const RowType* a, const RowType* b) const {
            return comp_(*a, *b);
        }
    };

public:
    // ==================== CONSTRUCTION ====================

    ModernTreeFilter() = default;

    explicit ModernTreeFilter(std::vector<RowType> data)
        : allData_(std::move(data)) {
        ResetFilter();
    }

    // ==================== DATA MANAGEMENT ====================

    void SetData(std::vector<RowType> data) {
        allData_ = std::move(data);
        ReapplyFilter();
    }

    void AddData(std::span<const RowType> newData) {
        allData_.insert(allData_.end(), newData.begin(), newData.end());
        ReapplyFilter();
    }

    std::span<const RowType> GetAllData() const {
        return std::span(allData_);
    }

    // ==================== FILTERING WITH RANGES ====================

    // Базовая фильтрация с предикатом
    template<RowPredicate<RowType> Pred>
    void Filter(Pred predicate) {
        currentPredicate_ = predicate;
        ApplyCurrentFilter();
    }

    // Фильтрация по тексту в любом текстовом поле
    void FilterByText(const std::wstring& searchText) {
        if (searchText.empty()) {
            ResetFilter();
            return;
        }

        auto lowerSearch = ToLowerCase(searchText);

        Filter([lowerSearch](const RowType& row) {
            return ContainsText(row, lowerSearch);
        });
    }

    // Фильтрация по диапазону ID
    void FilterByIdRange(std::optional<int> minId, std::optional<int> maxId) {
        Filter([minId, maxId](const RowType& row) {
            if (minId && row.id < *minId) return false;
            if (maxId && row.id > *maxId) return false;
            return true;
        });
    }

    // Комбинированная фильтрация (несколько условий)
    template<RowPredicate<RowType>... Preds>
    void FilterMultiple(Preds... predicates) {
        Filter([predicates...](const RowType& row) {
            return (predicates(row) && ...);  // Fold expression C++17/20
        });
    }

    // Сброс фильтра
    void ResetFilter() {
        currentPredicate_ = [](const RowType&) { return true; };
        ApplyCurrentFilter();
    }

    // ==================== SORTING ====================

    // Сортировка с компаратором
    template<RowComparator<RowType> Comp>
    void Sort(Comp comparator) {
        rngs::sort(filteredView_, comparator);
    }

    // Сортировка по полю (с проекцией)
    template<typename Proj>
        requires std::invocable<Proj, const RowType&>
    void SortByField(Proj projection, bool ascending = true) {
        if (ascending) {
            rngs::sort(filteredView_, std::less{},
                      [projection](const RowType* ptr) {
                          return projection(*ptr);
                      });
        } else {
            rngs::sort(filteredView_, std::greater{},
                      [projection](const RowType* ptr) {
                          return projection(*ptr);
                      });
        }
    }

    // Сортировка по ID
    void SortById(bool ascending = true) {
        SortByField([](const RowType& row) { return row.id; }, ascending);
    }

    // Сортировка по текстовому полю с обработкой optional
    void SortByVarchar(bool ascending = true) {
        SortByField([](const RowType& row) {
            return row.fVarchar.value_or(L"");
        }, ascending);
    }

    // ==================== ACCESS ====================

    size_t GetFilteredCount() const { return filteredView_.size(); }
    size_t GetTotalCount() const { return allData_.size(); }

    const RowType& GetFilteredRow(size_t index) const {
        return *filteredView_.at(index);
    }

    std::span<const RowType* const> GetFilteredView() const {
        return std::span(filteredView_);
    }

    // Получить диапазон для виртуальной прокрутки
    std::span<const RowType* const> GetRange(size_t start, size_t count) const {
        if (start >= filteredView_.size()) {
            return {};
        }
        size_t actualCount = std::min(count, filteredView_.size() - start);
        return std::span(filteredView_).subspan(start, actualCount);
    }

    // ==================== STATISTICS ====================

    // Получить статистику по ID (используя ranges)
    struct Statistics {
        int minId;
        int maxId;
        double avgId;
        size_t count;
    };

    std::optional<Statistics> GetStatistics() const {
        if (filteredView_.empty()) {
            return std::nullopt;
        }

        auto ids = filteredView_
            | views::transform([](const RowType* ptr) { return ptr->id; });

        auto [minIt, maxIt] = rngs::minmax_element(ids);

        int sum = 0;
        for (int id : ids) {
            sum += id;
        }

        return Statistics{
            .minId = *minIt,
            .maxId = *maxIt,
            .avgId = static_cast<double>(sum) / filteredView_.size(),
            .count = filteredView_.size()
        };
    }

private:
    // ==================== HELPER METHODS ====================

    void ApplyCurrentFilter() {
        filteredView_.clear();

        auto filtered = allData_
            | views::filter(currentPredicate_)
            | views::transform([](const RowType& row) -> const RowType* {
                return &row;
            });

        rngs::copy(filtered, std::back_inserter(filteredView_));
    }

    void ReapplyFilter() {
        if (currentPredicate_) {
            ApplyCurrentFilter();
        } else {
            ResetFilter();
        }
    }

    static std::wstring ToLowerCase(std::wstring str) {
        rngs::transform(str, str.begin(), ::towlower);
        return str;
    }

    static bool ContainsText(const RowType& row, const std::wstring& search) {
        if (row.fVarchar) {
            auto value = ToLowerCase(*row.fVarchar);
            return value.find(search) != std::wstring::npos;
        }
        return false;
    }
};

// ==================== TREE ADAPTER ====================

class ModernTreeAdapter {
private:
    ModernTreeFilter<generated::queries::TABLE_TEST_1_SOut> filter_;
    TVirtualStringTree* tree_;

    struct NodeData {
        size_t filteredIndex;  // Индекс в отфильтрованном представлении
    };

public:
    explicit ModernTreeAdapter(TVirtualStringTree* tree) : tree_(tree) {
        tree_->NodeDataSize = sizeof(NodeData);
    }

    // Установка данных
    void SetData(std::vector<generated::queries::TABLE_TEST_1_SOut> data) {
        filter_.SetData(std::move(data));
        UpdateTree();
    }

    // Применить фильтр по тексту
    void FilterByText(const std::wstring& text) {
        filter_.FilterByText(text);
        UpdateTree();
    }

    // Сортировка по колонке
    void SortByColumn(int column, bool ascending = true) {
        switch (column) {
            case 0: filter_.SortById(ascending); break;
            case 1: filter_.SortByVarchar(ascending); break;
            // Добавьте другие колонки...
        }
        tree_->Invalidate();
    }

    // Обработчик OnGetText
    void __fastcall OnGetText(TBaseVirtualTree* Sender, PVirtualNode Node,
                              TColumnIndex Column, TVSTTextType TextType,
                              UnicodeString& CellText) {
        auto* nodeData = static_cast<NodeData*>(Sender->GetNodeData(Node));

        try {
            const auto& row = filter_.GetFilteredRow(nodeData->filteredIndex);

            switch (Column) {
                case 0:
                    CellText = IntToStr(row.id);
                    break;
                case 1:
                    CellText = row.fVarchar.value_or(L"");
                    break;
                case 2:
                    if (row.fBigint) {
                        CellText = IntToStr(*row.fBigint);
                    }
                    break;
                // Добавьте обработку других колонок...
            }
        }
        catch (const std::out_of_range&) {
            CellText = L"<error>";
        }
    }

    // Обработчик OnInitNode
    void __fastcall OnInitNode(TBaseVirtualTree* Sender, PVirtualNode ParentNode,
                               PVirtualNode Node, TVirtualNodeInitStates& InitStates) {
        auto* nodeData = static_cast<NodeData*>(Sender->GetNodeData(Node));
        nodeData->filteredIndex = Node->Index;
    }

    // Получить статистику
    std::optional<ModernTreeFilter<generated::queries::TABLE_TEST_1_SOut>::Statistics>
    GetStatistics() const {
        return filter_.GetStatistics();
    }

private:
    void UpdateTree() {
        tree_->BeginUpdate();
        tree_->Clear();
        tree_->RootNodeCount = filter_.GetFilteredCount();
        tree_->EndUpdate();
    }
};

#endif // MODERN_TREE_FILTER_FULL_H
```

### Пример использования в форме

```cpp
// File: Example/uMainModernExample.h
#ifndef uMainModernExampleH
#define uMainModernExampleH

#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <VirtualTrees.hpp>
#include "modern_tree_filter_full.h"

class TfrmMainModern : public TForm {
__published:
    TVirtualStringTree *vsTree;
    TEdit *edtFilter;
    TButton *btnApplyFilter;
    TButton *btnResetFilter;
    TLabel *lblStatistics;
    TComboBox *cmbSortColumn;
    TCheckBox *chkAscending;

    void __fastcall FormCreate(TObject *Sender);
    void __fastcall btnApplyFilterClick(TObject *Sender);
    void __fastcall btnResetFilterClick(TObject *Sender);
    void __fastcall cmbSortColumnChange(TObject *Sender);
    void __fastcall vsTreeGetText(TBaseVirtualTree *Sender, PVirtualNode Node,
                                  TColumnIndex Column, TVSTTextType TextType,
                                  UnicodeString &CellText);
    void __fastcall vsTreeInitNode(TBaseVirtualTree *Sender, PVirtualNode ParentNode,
                                   PVirtualNode Node, TVirtualNodeInitStates &InitStates);
    void __fastcall vsTreeHeaderClick(TVTHeader *Sender, TColumnIndex Column,
                                      TMouseButton Button, TShiftState Shift,
                                      int X, int Y);

private:
    std::unique_ptr<ModernTreeAdapter> adapter_;

    void LoadData();
    void UpdateStatistics();

public:
    __fastcall TfrmMainModern(TComponent* Owner);
};

#endif
```

```cpp
// File: Example/uMainModernExample.cpp
#include <vcl.h>
#pragma hdrstop

#include "uMainModernExample.h"
#include "generated/queries/TABLE_TEST_1_SOut.h"

#pragma package(smart_init)
#pragma resource "*.dfm"

__fastcall TfrmMainModern::TfrmMainModern(TComponent* Owner)
    : TForm(Owner) {
}

void __fastcall TfrmMainModern::FormCreate(TObject *Sender) {
    // Создаем адаптер с C++20 возможностями
    adapter_ = std::make_unique<ModernTreeAdapter>(vsTree);

    // Подключаем события
    vsTree->OnGetText = vsTreeGetText;
    vsTree->OnInitNode = vsTreeInitNode;
    vsTree->Header->OnColumnClick = vsTreeHeaderClick;

    // Настраиваем колонки
    vsTree->Header->Columns->Clear();
    vsTree->Header->Columns->Add()->Text = L"ID";
    vsTree->Header->Columns->Add()->Text = L"VARCHAR";
    vsTree->Header->Columns->Add()->Text = L"BIGINT";
    vsTree->Header->Options = vsTree->Header->Options << hoVisible;

    // Загружаем данные
    LoadData();
}

void TfrmMainModern::LoadData() {
    // Создаем тестовые данные
    std::vector<generated::queries::TABLE_TEST_1_SOut> testData;
    testData.reserve(10000);

    for (int i = 0; i < 10000; ++i) {
        generated::queries::TABLE_TEST_1_SOut row;
        row.id = i;
        row.fVarchar = L"Строка " + IntToStr(i);
        row.fBigint = i * 100;
        testData.push_back(std::move(row));
    }

    adapter_->SetData(std::move(testData));
    UpdateStatistics();
}

void __fastcall TfrmMainModern::btnApplyFilterClick(TObject *Sender) {
    // Применяем фильтр с использованием ranges
    adapter_->FilterByText(edtFilter->Text.c_str());
    UpdateStatistics();
}

void __fastcall TfrmMainModern::btnResetFilterClick(TObject *Sender) {
    edtFilter->Text = L"";
    adapter_->FilterByText(L"");
    UpdateStatistics();
}

void __fastcall TfrmMainModern::cmbSortColumnChange(TObject *Sender) {
    int column = cmbSortColumn->ItemIndex;
    bool ascending = chkAscending->Checked;
    adapter_->SortByColumn(column, ascending);
}

void __fastcall TfrmMainModern::vsTreeGetText(TBaseVirtualTree *Sender,
                                              PVirtualNode Node,
                                              TColumnIndex Column,
                                              TVSTTextType TextType,
                                              UnicodeString &CellText) {
    adapter_->OnGetText(Sender, Node, Column, TextType, CellText);
}

void __fastcall TfrmMainModern::vsTreeInitNode(TBaseVirtualTree *Sender,
                                               PVirtualNode ParentNode,
                                               PVirtualNode Node,
                                               TVirtualNodeInitStates &InitStates) {
    adapter_->OnInitNode(Sender, ParentNode, Node, InitStates);
}

void __fastcall TfrmMainModern::vsTreeHeaderClick(TVTHeader *Sender,
                                                  TColumnIndex Column,
                                                  TMouseButton Button,
                                                  TShiftState Shift,
                                                  int X, int Y) {
    static bool ascending = true;
    adapter_->SortByColumn(Column, ascending);
    ascending = !ascending;  // Переключаем направление сортировки
}

void TfrmMainModern::UpdateStatistics() {
    auto stats = adapter_->GetStatistics();
    if (stats) {
        lblStatistics->Caption = UnicodeString::Format(
            L"Записей: %d | Min ID: %d | Max ID: %d | Avg ID: %.2f",
            ARRAYOFCONST((static_cast<int>(stats->count), stats->minId,
                         stats->maxId, stats->avgId))
        );
    } else {
        lblStatistics->Caption = L"Нет данных";
    }
}
```

---

## Преимущества C++20 подхода

### 1. **Производительность**
- Ranges используют ленивое вычисление (lazy evaluation)
- std::span не копирует данные
- Coroutines позволяют обрабатывать данные по мере поступления

### 2. **Безопасность типов**
- Concepts обеспечивают проверку во время компиляции
- Понятные сообщения об ошибках
- Невозможно передать неправильный тип

### 3. **Читаемость**
- Конвейеры ranges читаются как обычный текст
- Меньше вложенных циклов
- Декларативный стиль вместо императивного

### 4. **Гибкость**
- Легко комбинировать фильтры
- Простая композиция операций
- Переиспользование предикатов

---

## Сравнение: C++11 vs C++20

### Фильтрация - C++11 стиль
```cpp
void FilterByText(const std::wstring& text) {
    filteredView_.clear();
    for (const auto& row : allData_) {
        if (row.fVarchar) {
            std::wstring value = *row.fVarchar;
            std::transform(value.begin(), value.end(), value.begin(), ::towlower);
            if (value.find(text) != std::wstring::npos) {
                filteredView_.push_back(&row);
            }
        }
    }
}
```

### Фильтрация - C++20 стиль
```cpp
void FilterByText(const std::wstring& text) {
    filteredView_.clear();

    auto filtered = allData_
        | views::filter([&](const auto& row) {
            return ContainsText(row, text);
        })
        | views::transform([](const auto& row) { return &row; });

    rngs::copy(filtered, std::back_inserter(filteredView_));
}
```

---

## Требования компилятора

Для использования C++20 features в C++ Builder:

1. **C++ Builder 12.0 или новее** (RAD Studio 12.0)
2. Включить C++20 в настройках проекта:
   - Project → Options → C++ Compiler → Language
   - C++ Language Standard → C++20 (или /std:c++20)

3. Проверить доступность features:
```cpp
#if __cplusplus >= 202002L
    // C++20 доступен
    #include <ranges>
    #include <concepts>
    #include <span>
#else
    #error "C++20 required"
#endif
```

---

## Заключение

C++20 предоставляет мощные инструменты для работы с TVirtualTreeView:
- **Ranges** делают фильтрацию элегантной и эффективной
- **Concepts** обеспечивают безопасность типов
- **std::span** исключает ненужное копирование
- **Coroutines** позволяют работать с большими объемами данных асинхронно

Рекомендуется использовать Modern C++ подход для новых проектов!
