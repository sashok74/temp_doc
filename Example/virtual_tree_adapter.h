//---------------------------------------------------------------------------
#ifndef virtual_tree_adapterH
#define virtual_tree_adapterH
//---------------------------------------------------------------------------

#include <System.hpp>
#include <vector>
#include <VirtualTrees.hpp>
#include "../include/queries.hpp"

//---------------------------------------------------------------------------
// Virtual Tree Adapter - модуль для привязки данных к TVirtualStringTree
//
// Использование:
//   1. В FormCreate: VsTreeAdapter::SetupTreeForTableTest1(vsTree);
//   2. После получения данных: VsTreeAdapter::FillTreeWithTableTest1(vsTree, rows, cache);
//
// ============================================
// АДАПТАЦИЯ ПОД ДРУГУЮ СТРУКТУРУ:
// ============================================
// Чтобы использовать этот модуль с другой структурой данных:
//
// 1. Скопируйте функции SetupTreeFor* и FillTreeWith* с новым именем
// 2. Замените тип TABLE_TEST_1_SOut на вашу структуру
// 3. Измените количество колонок и их заголовки в SetupTreeFor*
// 4. Измените switch в OnGetText*, добавив case для ваших полей
// 5. Используйте форматтеры из value_formatters.hpp для преобразования
//
// Пример:
//   void SetupTreeForMyStruct(TVirtualStringTree* tree);
//   void FillTreeWithMyStruct(TVirtualStringTree* tree,
//       const std::vector<MyStruct>& rows, std::vector<MyStruct>& cache);
//---------------------------------------------------------------------------

namespace VsTreeAdapter {

    // Импортируем типы из generated::queries для удобства
    using generated::queries::TABLE_TEST_1_SOut;

    // Импортируем типы из Virtualtrees для event handlers
    using Virtualtrees::Basetree::TBaseVirtualTree;
    using Virtualtrees::Types::PVirtualNode;
    using Virtualtrees::Types::TVirtualNodeInitStates;
    using Virtualtrees::Types::TColumnIndex;
    using Virtualtrees::Types::TVSTTextType;
    using System::UnicodeString;

    //---------------------------------------------------------------------------
    // Структура данных узла - хранит только индекс строки в векторе
    //---------------------------------------------------------------------------
    struct NodeData {
        size_t rowIndex;
    };

    //---------------------------------------------------------------------------
    // Класс для хранения event handlers
    // Необходим, т.к. VCL event handlers требуют __closure (методы класса)
    //---------------------------------------------------------------------------
    class TTableTest1TreeHandler : public TObject
    {
    private:
        std::vector<TABLE_TEST_1_SOut>* cache_;
        std::vector<size_t> filteredIndices_;  // Индексы отфильтрованных строк
        TVirtualStringTree* tree_;              // Указатель на дерево для обновления

    public:
        __fastcall TTableTest1TreeHandler(std::vector<TABLE_TEST_1_SOut>* cache, TVirtualStringTree* tree)
            : cache_(cache), tree_(tree) {}

        void __fastcall OnInitNode(
            TBaseVirtualTree* Sender,
            PVirtualNode Parent,
            PVirtualNode Node,
            TVirtualNodeInitStates& InitStates);

        void __fastcall OnGetText(
            TBaseVirtualTree* Sender,
            PVirtualNode Node,
            TColumnIndex Column,
            TVSTTextType TextType,
            UnicodeString& CellText);

        void __fastcall OnFreeNode(
            TBaseVirtualTree* Sender,
            PVirtualNode Node);

        // ========== ФИЛЬТРАЦИЯ (C++20) ==========

        // Применить фильтр по тексту (поиск по всем текстовым полям)
        // Параметры:
        //   filterText - текст для поиска (без учета регистра, UTF-8)
        void ApplyFilter(const std::string& filterText);

        // Сбросить фильтр (показать все строки)
        void ResetFilter();

        // Получить количество отфильтрованных строк
        size_t GetFilteredCount() const { return filteredIndices_.size(); }
    };

    //---------------------------------------------------------------------------
    // SetupTreeForTableTest1
    //
    // Настраивает TVirtualStringTree для отображения данных TABLE_TEST_1_SOut:
    // - Устанавливает размер данных узла
    // - Создает 21 колонку с заголовками
    // - Создает и назначает обработчики событий
    // - Настраивает опции дерева
    // - Инициализирует поддержку фильтрации
    //
    // Параметры:
    //   tree  - указатель на TVirtualStringTree для настройки
    //   cache - указатель на вектор для кэширования данных
    //
    // Возвращает: указатель на handler объект (нужно сохранить для времени жизни!)
    //
    // АДАПТАЦИЯ: Скопируйте эту функцию и измените под вашу структуру
    //---------------------------------------------------------------------------
    TTableTest1TreeHandler* SetupTreeForTableTest1(
        TVirtualStringTree* tree,
        std::vector<TABLE_TEST_1_SOut>* cache
    );

    //---------------------------------------------------------------------------
    // FillTreeWithTableTest1
    //
    // Заполняет дерево данными из вектора TABLE_TEST_1_SOut:
    // - Копирует данные в cache (для обеспечения времени жизни)
    // - Создает узлы дерева (один узел = одна строка)
    //
    // Параметры:
    //   tree  - указатель на TVirtualStringTree
    //   rows  - вектор с данными для отображения
    //   cache - вектор для хранения копии данных (должен быть полем формы!)
    //
    // ВАЖНО: cache должен существовать все время жизни дерева,
    //        т.к. узлы хранят только индексы в этом векторе
    //
    // АДАПТАЦИЯ: Скопируйте эту функцию и измените тип структуры
    //---------------------------------------------------------------------------
    void FillTreeWithTableTest1(
        TVirtualStringTree* tree,
        const std::vector<TABLE_TEST_1_SOut>& rows,
        std::vector<TABLE_TEST_1_SOut>& cache
    );

} // namespace VsTreeAdapter

//---------------------------------------------------------------------------
#endif
