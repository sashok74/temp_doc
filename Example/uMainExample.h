//---------------------------------------------------------------------------

#ifndef uMainExampleH
#define uMainExampleH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.ComCtrls.hpp>
#include "VirtualTrees.AncestorVCL.hpp"
#include "VirtualTrees.BaseAncestorVCL.hpp"
#include "VirtualTrees.BaseTree.hpp"
#include "VirtualTrees.hpp"
#include <Vcl.ActnCtrls.hpp>
#include <Vcl.ActnMan.hpp>
#include <Vcl.ToolWin.hpp>
#include <vector>
#include "virtual_tree_adapter.h"
#include "../include/queries.hpp"

//---------------------------------------------------------------------------
class TfrmMainExample : public TForm
{
__published:	// IDE-managed Components
	TPanel *pBott;
	TButton *Button1;
	TPageControl *PageControl1;
	TTabSheet *TabSheet3;
	TVirtualStringTree *vsTree;
	void __fastcall Button1Click(TObject *Sender);
private:	// User declarations
	// Кэш данных для vsTree - должен существовать все время жизни дерева
	std::vector<generated::queries::TABLE_TEST_1_SOut> cachedTreeData_;
	// Handler для event callbacks vsTree
	VsTreeAdapter::TTableTest1TreeHandler* vsTreeHandler_;
protected:
	void __fastcall Loaded() override;
public:		// User declarations
	__fastcall TfrmMainExample(TComponent* Owner);
	__fastcall ~TfrmMainExample();
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmMainExample *frmMainExample;
//---------------------------------------------------------------------------
#endif
