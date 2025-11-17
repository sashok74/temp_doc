//---------------------------------------------------------------------------
#pragma hdrstop

#include "virtual_tree_adapter.h"
#include "value_formatters.hpp"

//---------------------------------------------------------------------------
#pragma package(smart_init)

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
// Реализация методов TTableTest1TreeHandler
//---------------------------------------------------------------------------

void __fastcall TTableTest1TreeHandler::OnInitNode(
    TBaseVirtualTree* Sender,
    PVirtualNode Parent,
    PVirtualNode Node,
    TVirtualNodeInitStates& InitStates)
{
    NodeData* data = static_cast<NodeData*>(Sender->GetNodeData(Node));
    if (data) {
        // Индекс узла = индекс в векторе (для корневых узлов)
        data->rowIndex = Node->Index;
    }
}

void __fastcall TTableTest1TreeHandler::OnGetText(
    TBaseVirtualTree* Sender,
    PVirtualNode Node,
    TColumnIndex Column,
    TVSTTextType TextType,
    UnicodeString& CellText)
{
    // Проверка кэша - если пустой, вернуть пустую строку (не вызывать ошибку)
    if (!cache_ || cache_->empty()) {
        CellText = L"";
        return;
    }

    // Получить данные узла
    NodeData* nodeData = static_cast<NodeData*>(Sender->GetNodeData(Node));
    if (!nodeData || nodeData->rowIndex >= cache_->size()) {
        CellText = L"<invalid>";
        return;
    }

    // Получить строку данных
    const TABLE_TEST_1_SOut& row = (*cache_)[nodeData->rowIndex];

    // ========== АДАПТАЦИЯ: Switch по колонкам ==========
    switch (Column) {
        case 0:  CellText = example::format::FormatNumericValue(row.id); break;
        case 1:  CellText = example::format::FormatNumericOptional(row.fBigint); break;
        case 2:  CellText = example::format::FormatBoolOptional(row.fBoolean); break;
        case 3:  CellText = example::format::FormatStringOptional(row.fChar); break;
        case 4:  CellText = example::format::FormatDateOptional(row.fDate); break;
        case 5:  CellText = example::format::FormatDecFloatOptional(row.fDecfloat); break;
        case 6:  CellText = example::format::FormatTTNumericOptional(row.fDecimal); break;
        case 7:  CellText = example::format::FormatNumericOptional(row.fDoublePrecision); break;
        case 8:  CellText = example::format::FormatNumericOptional(row.fFloat); break;
        case 9:  CellText = example::format::FormatInt128Optional(row.fInt128); break;
        case 10: CellText = example::format::FormatNumericOptional(row.fInteger); break;
        case 11: CellText = example::format::FormatTTNumericOptional(row.fNumeric); break;
        case 12: CellText = example::format::FormatNumericOptional(row.fSmalint); break;
        case 13: CellText = example::format::FormatTimeOptional(row.fTime); break;
        case 14: CellText = example::format::FormatTimeTzOptional(row.fTimeTz); break;
        case 15: CellText = example::format::FormatTimestampOptional(row.fTimeshtamp); break;
        case 16: CellText = example::format::FormatTimestampTzOptional(row.fTimeshtampTz); break;
        case 17: CellText = example::format::FormatStringOptional(row.fVarchar); break;
        case 18: CellText = example::format::FormatBlobOptional(row.fBlobB); break;
        case 19: CellText = example::format::FormatTextBlobOptional(row.fBlobT); break;
        case 20: CellText = example::format::FormatNumericOptional(row.fNull); break;
        default: CellText = L""; break;
    }
    // ===================================================
}

void __fastcall TTableTest1TreeHandler::OnFreeNode(
    TBaseVirtualTree* Sender,
    PVirtualNode Node)
{
    // NodeData содержит только size_t, не требует деструкции
}

//---------------------------------------------------------------------------
// SetupTreeForTableTest1
//
// АДАПТАЦИЯ: При копировании для другой структуры:
// 1. Измените имя функции и класса handler
// 2. Измените количество колонок (COLUMN_COUNT)
// 3. Измените заголовки колонок (columnHeaders)
//---------------------------------------------------------------------------
TTableTest1TreeHandler* SetupTreeForTableTest1(
    TVirtualStringTree* tree,
    std::vector<TABLE_TEST_1_SOut>* cache)
{
    if (!tree || !cache) {
        return nullptr;
    }

    // Проверка что tree->Header инициализирован
    if (!tree->Header) {
        throw Exception("VirtualStringTree Header is not initialized - missing #pragma link \"VirtualTrees\"?");
    }

    // Создать handler объект
    TTableTest1TreeHandler* handler = new TTableTest1TreeHandler(cache);


    // Размер данных узла
    tree->NodeDataSize = sizeof(NodeData);

    // ========== АДАПТАЦИЯ: Количество и названия колонок ==========
    const int COLUMN_COUNT = 21;
    const UnicodeString columnHeaders[COLUMN_COUNT] = {
        L"ID",
        L"F_BIGINT",
        L"F_BOOLEAN",
        L"F_CHAR",
        L"F_DATE",
        L"F_DECFLOAT",
        L"F_DECIMAL",
        L"F_DOUBLE",
        L"F_FLOAT",
        L"F_INT128",
        L"F_INTEGER",
        L"F_NUMERIC",
        L"F_SMALINT",
        L"F_TIME",
        L"F_TIME_TZ",
        L"F_TIMESTAMP",
        L"F_TIMESTAMP_TZ",
        L"F_VARCHAR",
        L"F_BLOB_B",
        L"F_BLOB_T",
        L"F_NULL"
    };
    // ===============================================================

    // Блокируем обновление дерева во время настройки
    // Это предотвращает вызов event handlers до завершения настройки
    tree->BeginUpdate();
	try {
        // ВАЖНО: Очистить дерево ДО назначения event handlers
        // Это предотвращает вызов OnGetText для существующих узлов с пустым кэшем
        tree->Clear();

        // Очистить существующие колонки
        tree->Header->Columns->Clear();

		// Создать колонки
		for (int i = 0; i < COLUMN_COUNT; i++) {
			TVirtualTreeColumn* col = tree->Header->Columns->Add();
			col->Text = columnHeaders[i];
			col->Width = 100; // Начальная ширина
			col->Options = col->Options << Virtualtrees::Types::coVisible << Virtualtrees::Types::coEnabled;
		}


		// Настройка Header
		tree->Header->AutoSizeIndex = -1; // Не растягивать автоматически

		// ВАЖНО: Создаём новый Set объект вместо модификации существующего свойства
		// Это избегает промежуточных вызовов setter'ов, которые могут вызвать AV
		TVTHeaderOptions headerOpts;
		headerOpts << Virtualtrees::Types::hoVisible
				   << Virtualtrees::Types::hoColumnResize
				   << Virtualtrees::Types::hoShowSortGlyphs;
		tree->Header->Options = headerOpts;

		tree->Header->Style = Virtualtrees::Types::hsThickButtons;
		tree->Header->MainColumn = 0;

		// Настройка Tree Options
		tree->TreeOptions->SelectionOptions = tree->TreeOptions->SelectionOptions
			<< Virtualtrees::Types::toFullRowSelect;
		tree->TreeOptions->MiscOptions = tree->TreeOptions->MiscOptions
			<< Virtualtrees::Types::toGridExtensions;
		tree->TreeOptions->PaintOptions = tree->TreeOptions->PaintOptions
			<< Virtualtrees::Types::toShowHorzGridLines << Virtualtrees::Types::toShowVertGridLines;

		// Назначить обработчики событий
		// ВАЖНО: назначаем в BeginUpdate, чтобы они не вызывались до EndUpdate
		tree->OnInitNode = handler->OnInitNode;
		tree->OnGetText = handler->OnGetText;
		tree->OnFreeNode = handler->OnFreeNode;
    }
    __finally {
        // Завершаем блокировку - теперь дерево готово к работе
        tree->EndUpdate();
    }

    return handler;
}

//---------------------------------------------------------------------------
// FillTreeWithTableTest1
//
// АДАПТАЦИЯ: При копировании для другой структуры:
// 1. Измените имя функции
// 2. Измените тип параметров rows и cache на вашу структуру
//---------------------------------------------------------------------------
void FillTreeWithTableTest1(
    TVirtualStringTree* tree,
    const std::vector<TABLE_TEST_1_SOut>& rows,
    std::vector<TABLE_TEST_1_SOut>& cache)
{
    if (!tree) {
        return;
    }

    // Копируем данные в cache
    cache = rows;

    // Сохраняем указатель на cache в Tag дерева
    // ВАЖНО: cache должен существовать все время жизни дерева!
    tree->Tag = reinterpret_cast<NativeInt>(&cache);

    // Заполняем дерево
    tree->BeginUpdate();
    try {
        tree->Clear();
        tree->RootNodeCount = cache.size();
    }
    __finally {
        tree->EndUpdate();
    }
}

} // namespace VsTreeAdapter
