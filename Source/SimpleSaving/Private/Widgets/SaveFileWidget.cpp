// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Widgets/SaveFileWidget.h"

//=================================================================
// 
//=================================================================
void USaveFileWidget::Setup(const FSaveFileList &InFile)
{
	Filename = InFile.Filename;
	DateTime = InFile.DateTime;
	OnSetup();
}

//=================================================================
// 
//=================================================================
void USaveFileWidget::Click()
{
	OnClicked();
	OnClick.Broadcast(Filename);
}