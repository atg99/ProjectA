// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ATGShopWidget.h"
#include "ATGInventoryGridWidget.h"

bool UATGShopWidget::NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UE_LOG(LogTemp, Display, TEXT("UATGInventoryWidget::NativeOnDrop"));

	// 그리드 상위 위젯이 다른 그리드에서 온 drag drop 이벤트 처리   
	// InOperation에 있는 TScriptInterface로 자기 그리드에서온 이벤트인지 검사후 다른 그리드면 TryAddItemAt 서버 RPC전송
	// 후 이벤트 소스 그리드는 수량감소, 받은 그리드는 수량 증가

	//PlayerGrid->NativeOnDrop(InGeo, InDragDropEvent, InOperation);
	//TransactionGrid->NativeOnDrop(InGeo, InDragDropEvent, InOperation);
	//MerchantGrid->NativeOnDrop(InGeo, InDragDropEvent, InOperation);

	return false;
}
