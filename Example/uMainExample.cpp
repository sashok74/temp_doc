//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "uMainExample.h"

#include "../include/queries.hpp"
#include "fbpp/core/connection.hpp"
#include "value_formatters.hpp"
#include <System.SysUtils.hpp>
#include <chrono>
#include <memory>
#include <vector>

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "VirtualTrees"
#pragma link "VirtualTrees.AncestorVCL"
#pragma link "VirtualTrees.BaseAncestorVCL"
#pragma link "VirtualTrees.BaseTree"
#pragma resource "*.dfm"
TfrmMainExample *frmMainExample;
//---------------------------------------------------------------------------

#pragma comment(lib, "fbclient_ms.lib")
#pragma comment(lib, "ib_util_ms.lib")
#pragma comment(lib, "fbpp_core.lib")
#pragma comment(lib, "CppDecimal.lib")

namespace
{
	using generated::queries::QueryDescriptor;
	using generated::queries::QueryId;
	using generated::queries::TABLE_TEST_1_SIn;
	using generated::queries::TABLE_TEST_1_SOut;

	constexpr const char* kDatabase = "firebird5:testdb";
	constexpr const char* kUser = "SYSDBA";
	constexpr const char* kPassword = "planomer";
	constexpr const char* kCharset = "UTF8";
}

//---------------------------------------------------------------------------
__fastcall TfrmMainExample::TfrmMainExample(TComponent* Owner)
	: TForm(Owner), vsTreeHandler_(nullptr)
{

}
//---------------------------------------------------------------------------

void __fastcall TfrmMainExample::Loaded()
{
	TForm::Loaded();

	// ������ vsTree ��������� ��������������� � ����� � ���������
	if (!vsTreeHandler_ && vsTree) {
		vsTreeHandler_ = VsTreeAdapter::SetupTreeForTableTest1(vsTree, &cachedTreeData_);
	   //	vsTree->Header->ParentFont = false;
	   //	vsTree->Header->Font->Assign(Font);
	  	vsTree->TreeOptions->AutoOptions = vsTree->TreeOptions->AutoOptions >> Virtualtrees::Types::toAutoChangeScale;
	}
}
//---------------------------------------------------------------------------

__fastcall TfrmMainExample::~TfrmMainExample()
{
	// ������� handler ������
	if (vsTreeHandler_) {
		delete vsTreeHandler_;
		vsTreeHandler_ = nullptr;
	}
}
//---------------------------------------------------------------------------


void __fastcall TfrmMainExample::Button1Click(TObject *Sender)
{
	try
	{
		fbpp::core::ConnectionParams params;
		params.database = kDatabase;
		params.user = kUser;
		params.password = kPassword;
		params.charset = kCharset;

		fbpp::core::Connection connection(params);
		auto transaction = connection.StartTransaction();

		TABLE_TEST_1_SIn input{};
		input.param1 = 0; // fetch rows with ID > 0

		using Descriptor = QueryDescriptor<QueryId::TABLE_TEST_1_S>;
		const auto rows = fbpp::core::executeQuery<Descriptor>(connection, *transaction, input);
		transaction->Commit();

		// ���������� vsTree �������
		VsTreeAdapter::FillTreeWithTableTest1(vsTree, rows, cachedTreeData_);
	}
	catch (const std::exception& e)
	{}
}
//---------------------------------------------------------------------------

// ========== РЕАЛИЗАЦИЯ ФИЛЬТРАЦИИ ==========

void TfrmMainExample::FilterData(const std::string& filterText)
{
	if (!vsTreeHandler_) {
		return;
	}

	// Применяем фильтр через handler
	// Фильтрация выполняется по всем полям (регистронезависимо)
	vsTreeHandler_->ApplyFilter(filterText);
}

void TfrmMainExample::ResetFilter()
{
	if (!vsTreeHandler_) {
		return;
	}

	// Сбрасываем фильтр через handler
	vsTreeHandler_->ResetFilter();
}
//---------------------------------------------------------------------------













