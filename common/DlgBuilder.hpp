#pragma once

/*
DlgBuilder.hpp

Динамическое конструирование диалогов
*/
/*
Copyright © 2009 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef UNICODE
#define EMPTY_TEXT L""
#else
#define EMPTY_TEXT ""
#endif

#if FAR_UNICODE>=1906
extern GUID guid_PluginGuid;
#define FarDlgProcParam2 void*
#else
#define FARDIALOGITEMTYPES int
#define FarDlgProcParam2 LONG_PTR
#endif

// Элемент выпадающего списка в диалоге.
struct DialogBuilderListItem
{
	// Строчка из LNG-файла, которая будет показана в диалоге.
	int MessageId;

	// Значение, которое будет записано в поле Value при выборе этой строчки.
	int ItemValue;
};

template<class T>
struct DialogItemBinding
{
	int BeforeLabelID;
	int AfterLabelID;

	DialogItemBinding()
		: BeforeLabelID(-1), AfterLabelID(-1)
	{
	}

	virtual void SaveValue(T *Item, int RadioGroupIndex)
	{
	}
};

template<class T>
struct CheckBoxBinding: public DialogItemBinding<T>
{
	private:
		BOOL *Value;
		int Mask;

	public:
		CheckBoxBinding(BOOL *aValue, int aMask) : Value(aValue), Mask(aMask) { }

		virtual void SaveValue(T *Item, int RadioGroupIndex)
		{
			if (!Mask)
			{
				*Value = Item->Selected;
			}
			else
			{
				if (Item->Selected)
					*Value |= Mask;
				else
					*Value &= ~Mask;
			}
		}
};

template<class T>
struct RadioButtonBinding: public DialogItemBinding<T>
{
	private:
		int *Value;

	public:
		RadioButtonBinding(int *aValue) : Value(aValue) { }

		virtual void SaveValue(T *Item, int RadioGroupIndex)
		{
			if (Item->Selected)
				*Value = RadioGroupIndex;
		}
};

template<class T>
struct ComboBoxBinding: public DialogItemBinding<T>
{
	int *Value;
	FarList *List;

	ComboBoxBinding(int *aValue, FarList *aList)
		: Value(aValue), List(aList)
	{
	}

	virtual ~ComboBoxBinding()
	{
		delete [] List->Items;
		delete List;
	}

	virtual void SaveValue(T *Item, int RadioGroupIndex)
	{
		FarListItem &ListItem = List->Items[Item->ListPos];
		*Value = ListItem.Reserved[0];
	}
};

/*
Класс для динамического построения диалогов. Автоматически вычисляет положение и размер
для добавляемых контролов, а также размер самого диалога. Автоматически записывает выбранные
значения в указанное место после закрытия диалога по OK.

По умолчанию каждый контрол размещается в новой строке диалога. Ширина для текстовых строк,
checkbox и radio button вычисляется автоматически, для других элементов передаётся явно.
Есть также возможность добавить статический текст слева или справа от контрола, при помощи
методов AddTextBefore и AddTextAfter.

Поддерживается также возможность расположения контролов в две колонки. Используется следующим
образом:
- StartColumns()
- добавляются контролы для первой колонки
- ColumnBreak()
- добавляются контролы для второй колонки
- EndColumns()

Поддерживается также возможность расположения контролов внутри бокса. Используется следующим
образом:
- StartSingleBox()
- добавляются контролы
- EndSingleBox()

Базовая версия класса используется как внутри кода Far, так и в плагинах.
*/

template<class T>
class DialogBuilderBase
{
	protected:
		T *DialogItems;
		DialogItemBinding<T> **Bindings;
		int DialogItemsCount;
		int DialogItemsAllocated;
		int NextY;
		int Indent;
		int SingleBoxIndex;
		int OKButtonID, CancelButtonID;
		int ColumnStartIndex;
		int ColumnBreakIndex;
		int ColumnStartY;
		int ColumnEndY;
		int ColumnMinWidth;
		GUID DlgGuid;

		static const int SECOND_COLUMN = -2;
		static const int RIGHT_SLIDE = -3;

		void ReallocDialogItems()
		{
			// реаллокация инвалидирует указатели на DialogItemEx, возвращённые из
			// AddDialogItem и аналогичных методов, поэтому размер массива подбираем такой,
			// чтобы все нормальные диалоги помещались без реаллокации
			// TODO хорошо бы, чтобы они вообще не инвалидировались
			DialogItemsAllocated += 128;
			if (!DialogItems)
			{
				DialogItems = new T[DialogItemsAllocated];
				Bindings = new DialogItemBinding<T> * [DialogItemsAllocated];
			}
			else
			{
				T *NewDialogItems = new T[DialogItemsAllocated];
				DialogItemBinding<T> **NewBindings = new DialogItemBinding<T> * [DialogItemsAllocated];
				for(int i=0; i<DialogItemsCount; i++)
				{
					NewDialogItems [i] = DialogItems [i];
					NewBindings [i] = Bindings [i];
				}
				delete [] DialogItems;
				delete [] Bindings;
				DialogItems = NewDialogItems;
				Bindings = NewBindings;
			}
		}

		T *AddDialogItem(FARDIALOGITEMTYPES Type, const TCHAR *Text)
		{
			if (DialogItemsCount == DialogItemsAllocated)
			{
				// Иначе могут потеряться указатели, которые запомнены в вызывающем плагине
				_ASSERTE(!DialogItemsAllocated || DialogItemsCount < DialogItemsAllocated);
				ReallocDialogItems();
			}
			int Index = DialogItemsCount++;
			T *Item = &DialogItems [Index];
			InitDialogItem(Item, Text);
			Item->Type = Type;
			Bindings [Index] = nullptr;
			return Item;
		}

public:
		void SetNextY(T *Item)
		{
			Item->X1 = 5 + Indent;
			Item->Y1 = Item->Y2 = NextY++;
		}

		void SlideItemUp(T *Item)
		{
			int Width = Item->X2 - Item->X1;
			Item->X1 = RIGHT_SLIDE;
			Item->X2 = Item->X1 + Width;
			NextY--;
			Item->Y1 = Item->Y2 = NextY - 1;
		}

		int ItemWidth(const T &Item)
		{
			int Width = 0;
			switch(Item.Type)
			{
			case DI_TEXT:
				if (Item.X2)
					Width = Item.X2 - Item.X1;
				else
					Width = TextWidth(Item);
				break;

			case DI_CHECKBOX:
			case DI_RADIOBUTTON:
			case DI_BUTTON:
				Width = TextWidth(Item) + 4;
				break;

			case DI_EDIT:
			case DI_FIXEDIT:
			case DI_COMBOBOX:
			case DI_PSWEDIT:
				Width = Item.X2 - Item.X1 + 1;
				/* стрелка history занимает дополнительное место, но раньше она рисовалась поверх рамки
				if (Item.Flags & DIF_HISTORY)
					Width++;
				*/
			}
			return Width;
		}

		void AddBorder(const TCHAR *TitleText)
		{
			T *Title = AddDialogItem(DI_DOUBLEBOX, TitleText);
			Title->X1 = 3;
			Title->Y1 = 1;
		}

		void UpdateBorderSize()
		{
			T *Title = &DialogItems[0];
			Title->X2 = Title->X1 + MaxTextWidth() + 3;
			Title->Y2 = DialogItems [DialogItemsCount-1].Y2 + 1;

			for (int i=1; i<DialogItemsCount; i++)
			{
				if (DialogItems[i].Type == DI_SINGLEBOX)
				{
					Indent = 2;
					DialogItems[i].X2 = Title->X2;
				}
			}

			Title->X2 += Indent;
			Indent = 0;
		}

		int MaxTextWidth()
		{
			int MaxWidth = 0;
			for(int i=1; i<DialogItemsCount; i++)
			{
				if (DialogItems [i].X1 == SECOND_COLUMN)
					continue;
				int Width = ItemWidth(DialogItems [i]);
				int Indent = DialogItems [i].X1 - 5;
				Width += Indent;

				if (MaxWidth < Width)
					MaxWidth = Width;
			}
			int ColumnsWidth = 2*ColumnMinWidth+1;
			if (MaxWidth < ColumnsWidth)
				return ColumnsWidth;
			return MaxWidth;
		}

		void UpdateSecondColumnPosition()
		{
			int SecondColumnX1 = 6 + (DialogItems [0].X2 - DialogItems [0].X1 - 1)/2;
			for(int i=0; i<DialogItemsCount; i++)
			{
				if (DialogItems [i].X1 == SECOND_COLUMN)
				{
					DialogItemBinding<T> *Binding = FindBinding(DialogItems+i);
					int BeforeWidth = 0;
					if (Binding && Binding->BeforeLabelID != -1) {
						BeforeWidth = DialogItems [Binding->BeforeLabelID].X2 - DialogItems [Binding->BeforeLabelID].X1 + 1;
					}

					int Width = DialogItems [i].X2 - DialogItems [i].X1;
					DialogItems [i].X1 = SecondColumnX1 + BeforeWidth;
					DialogItems [i].X2 = DialogItems [i].X1 + Width;

					if (Binding && Binding->AfterLabelID != -1)
					{
						int j = Binding->AfterLabelID;
						int AfterWidth = DialogItems [j].X2 - DialogItems [j].X1;
						if (DialogItems [j].X1 == SECOND_COLUMN)
						{
							DialogItems [j].X1 = SecondColumnX1 + Width + 2;
							DialogItems [j].X2 = DialogItems [j].X1 + AfterWidth;
						}
						else
						{
							DialogItems [j].X1 += Width + 2;
							DialogItems [j].X2 += Width + 2;
						}
					}
				}
			}
		}

		void UpdateRightSlidePosition()
		{
			int SlideX2 = DialogItems [0].X2 - 2;
			for(int i=0; i<DialogItemsCount; i++)
			{
				if (DialogItems [i].X1 == RIGHT_SLIDE)
				{
					int Width = DialogItems [i].X2 - DialogItems [i].X1;
					DialogItems [i].X1 = SlideX2 - Width;
					DialogItems [i].X2 = DialogItems [i].X1 + Width;
				}
				else if ((DialogItems[i].Flags & DIF_CENTERTEXT) && DialogItems [i].X2 == 0)
				{
					DialogItems [i].X2 = DialogItems [0].X2 - 2;
				}
			}
		}

		virtual void InitDialogItem(T *NewDialogItem, const TCHAR *Text)
		{
		}

		virtual int TextWidth(const T &Item)
		{
			return -1;
		}

		void SetLastItemBinding(DialogItemBinding<T> *Binding)
		{
			Bindings [DialogItemsCount-1] = Binding;
		}

		int GetItemID(T *Item)
		{
			int Index = static_cast<int>(Item - DialogItems);
			if (Index >= 0 && Index < DialogItemsCount)
				return Index;
			return -1;
		}

		DialogItemBinding<T> *FindBinding(const T *Item)
		{
			int Index = static_cast<int>(Item - DialogItems);
			if (Index >= 0 && Index < DialogItemsCount)
				return Bindings [Index];
			return nullptr;
		}

		void SaveValues()
		{
			int RadioGroupIndex = 0;
			for(int i=0; i<DialogItemsCount; i++)
			{
				if (DialogItems [i].Flags & DIF_GROUP)
					RadioGroupIndex = 0;
				else
					RadioGroupIndex++;

				if (Bindings [i])
					Bindings [i]->SaveValue(&DialogItems [i], RadioGroupIndex);
			}
		}

		virtual const TCHAR *GetLangString(int MessageID)
		{
			return nullptr;
		}

		virtual int DoShowDialog()
		{
			return -1;
		}

		virtual DialogItemBinding<T> *CreateCheckBoxBinding(BOOL *Value, int Mask)
		{
			return nullptr;
		}

		virtual DialogItemBinding<T> *CreateRadioButtonBinding(int *Value)
		{
			return nullptr;
		}

		DialogBuilderBase(const GUID* aDlgGuid)
			: DialogItems(nullptr), DialogItemsCount(0), DialogItemsAllocated(0), NextY(2), Indent(0), SingleBoxIndex(-1),
			  ColumnStartIndex(-1), ColumnBreakIndex(-1), ColumnMinWidth(0), DlgGuid(*aDlgGuid)
		{
		}

		virtual ~DialogBuilderBase()
		{
			for(int i=0; i<DialogItemsCount; i++)
			{
				if (Bindings [i])
					delete Bindings [i];
			}
			delete [] DialogItems;
			delete [] Bindings;
		}

	public:

		int GetLastID()
		{
			return DialogItemsCount-1;
		}

		// Добавляет статический текст, расположенный на отдельной строке в диалоге.
		T *AddText(int LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, LabelId == -1 ? EMPTY_TEXT : GetLangString(LabelId));
			SetNextY(Item);
			return Item;
		}

		// Добавляет статический текст, расположенный на отдельной строке в диалоге.
		T *AddText(const TCHAR* Label)
		{
			T *Item = AddDialogItem(DI_TEXT, Label);
			SetNextY(Item);
			return Item;
		}
		//void CenterText(T* Item)
		//{
		//	Item->Flags |= DIF_CENTERTEXT;
		//	Item->X2 = Item->X1 + ItemWidth(*Item);
		//}

		// Добавляет чекбокс.
		T *AddCheckbox(int TextMessageId, BOOL *Value, int Mask=0, bool ThreeState=false)
		{
			T *Item = AddDialogItem(DI_CHECKBOX, GetLangString(TextMessageId));
			if (ThreeState && !Mask)
				Item->Flags |= DIF_3STATE;
			SetNextY(Item);
			Item->X2 = Item->X1 + ItemWidth(*Item);
			if (!Mask)
				Item->Selected = *Value;
			else
				Item->Selected = (*Value & Mask) ? TRUE : FALSE ;
			SetLastItemBinding(CreateCheckBoxBinding(Value, Mask));
			return Item;
		}

		// Добавляет группу радиокнопок.
		// Возвращает ИД(Index) первой добавленной кнопки
		int AddRadioButtons(int *Value, int OptionCount, int MessageIDs[], bool FocusOnSelected=false, unsigned __int64 AddFlags=0, BOOL abTwoColumns = FALSE)
		{
			if (abTwoColumns && (ColumnStartIndex != -1 || OptionCount < 2))
				abTwoColumns = FALSE;
			if (abTwoColumns)
				StartColumns();

			int nBreakColumn = abTwoColumns ? (((OptionCount+1)>>1)-1) : (OptionCount+1);
			int nFirstID = DialogItemsCount;
			for(int i=0; i<OptionCount; i++)
			{
				T *Item = AddDialogItem(DI_RADIOBUTTON, GetLangString(MessageIDs[i]));
				SetNextY(Item);
				Item->X2 = Item->X1 + ItemWidth(*Item);
				if (!i)
					Item->Flags |= DIF_GROUP;
				if (AddFlags)
					Item->Flags |= AddFlags;
				if (*Value == i)
				{
					Item->Selected = TRUE;
					#if FAR_UNICODE>=1988
					if (FocusOnSelected)
						Item->Flags |= DIF_FOCUS;
					#endif
				}
				SetLastItemBinding(CreateRadioButtonBinding(Value));
				if (i == nBreakColumn)
					ColumnBreak();
			}
			if (abTwoColumns)
				EndColumns();
			return nFirstID;
		}

		// Добавляет ComboBox
		T* AddComboBox(int Width, FarList* ListItems, int *Value, DWORD AddFlags=DIF_DROPDOWNLIST)
		{
			T *Item = AddDialogItem(DI_COMBOBOX, nullptr);

			for(int i = 0; i < ListItems->ItemsNumber; i++)
			{
				if(i == *Value)
					ListItems->Items[i].Flags |= LIF_SELECTED;
				else if(ListItems->Items[i].Flags & LIF_SELECTED)
					ListItems->Items[i].Flags &= ~LIF_SELECTED;
			}

			Item->ListItems = ListItems;
			Item->Flags |= AddFlags;
			SetNextY(Item);
			Item->X2 = Item->X1 + Width;
			SetLastItemBinding(CreateComboBoxBinding(Value));
			return Item;
		}

		// Добавляет поле типа DI_FIXEDIT для редактирования указанного числового значения.
		virtual T *AddIntEditField(int *Value, int Width)
		{
			return nullptr;
		}

		// Добавляет указанную текстовую строку слева от элемента RelativeTo.
		T *AddTextBefore(T *RelativeTo, int LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, GetLangString(LabelId));
			Item->Y1 = Item->Y2 = RelativeTo->Y1;
			Item->X1 = 5 + Indent;
			Item->X2 = Item->X1 + ItemWidth(*Item) - 1;

			int RelativeToWidth = RelativeTo->X2 - RelativeTo->X1;
			RelativeTo->X1 = Item->X2 + 2;
			RelativeTo->X2 = RelativeTo->X1 + RelativeToWidth;

			//// Текст должен идти первым элементом, чтобы хоткеи срабатывали
			//T P; P = *RelativeTo; *RelativeTo = *Item; *Item = P;
			//Item--; RelativeTo++;

			//int I1 = static_cast<int>(Item - DialogItems);
			//int I2 = static_cast<int>(RelativeTo - DialogItems);
			//DialogItemBinding<T> *B1;
			//B1 = Bindings[I1]; Bindings[I1] = Bindings[I2]; Bindings[I2] = B1;

			DialogItemBinding<T> *Binding = FindBinding(RelativeTo);
			if (Binding)
				Binding->BeforeLabelID = GetItemID(Item);

			return Item;
		}

		// Добавляет указанную текстовую строку справа от элемента RelativeTo.
		T *AddTextAfter(T *RelativeTo, int LabelId)
		{
			T *Item = AddDialogItem(DI_TEXT, GetLangString(LabelId));
			Item->Y1 = Item->Y2 = RelativeTo->Y1;
			Item->X1 = RelativeTo->X1 + ItemWidth(*RelativeTo) - 1 + 2;

			DialogItemBinding<T> *Binding = FindBinding(RelativeTo);
			if (Binding)
				Binding->AfterLabelID = GetItemID(Item);

			return Item;
		}

		// Добавляет кнопку справа от элемента RelativeTo.
		T *AddButtonAfter(T *RelativeTo, int LabelId)
		{
			T *Item = AddDialogItem(DI_BUTTON, GetLangString(LabelId));
			Item->Y1 = Item->Y2 = RelativeTo->Y1;
			Item->X1 = RelativeTo->X1 + ItemWidth(*RelativeTo) - 1 + 2;

			DialogItemBinding<T> *Binding = FindBinding(RelativeTo);
			if (Binding)
				Binding->AfterLabelID = GetItemID(Item);

			return Item;
		}

		// Подвязать (a'la AddTextAfter) любой элемент к любому элементу
		void MoveItemAfter(T *FirstItem, T *AfterItem)
		{
			int Width  = ItemWidth(*AfterItem);
			int Height = AfterItem->Y2 - AfterItem->Y1;
			
			AfterItem->Y1 = FirstItem->Y1;
			AfterItem->Y2 = FirstItem->Y1 + Height;
			AfterItem->X1 = FirstItem->X1 + ItemWidth(*FirstItem);
			AfterItem->X2 = AfterItem->X1 + Width - 1;

			DialogItemBinding<T> *Binding = FindBinding(AfterItem);
			if (Binding)
			{
				Binding->BeforeLabelID = GetItemID(FirstItem);
				_ASSERTE(Binding->BeforeLabelID != -1);
			}

			NextY--;
		}

		// Начинает располагать поля диалога в две колонки.
		void StartColumns()
		{
			ColumnStartIndex = DialogItemsCount;
			ColumnStartY = NextY;
		}

		// Завершает колонку полей в диалоге и переходит к следующей колонке.
		void ColumnBreak()
		{
			ColumnBreakIndex = DialogItemsCount;
			ColumnEndY = NextY;
			NextY = ColumnStartY;
		}

		// Завершает расположение полей диалога в две колонки.
		void EndColumns()
		{
			for(int i=ColumnStartIndex; i<DialogItemsCount; i++)
			{
				int Width = ItemWidth(DialogItems [i]);

				DialogItemBinding<T> *Binding = FindBinding(DialogItems + i);
				int BindingAdd = 0;
				if (Binding) {
					if (Binding->BeforeLabelID != -1)
						BindingAdd = ItemWidth(DialogItems [Binding->BeforeLabelID]) + 1;
					else if (Binding->AfterLabelID != -1)
						BindingAdd = ItemWidth(DialogItems [Binding->AfterLabelID]) + 1;
				}

				if ((Width + BindingAdd) > ColumnMinWidth)
					ColumnMinWidth = Width + BindingAdd;
				if (i >= ColumnBreakIndex)
				{
					DialogItems [i].X1 = SECOND_COLUMN;
					DialogItems [i].X2 = SECOND_COLUMN + Width;
				}
			}

			if (ColumnEndY > NextY)
				NextY = ColumnEndY;

			ColumnStartIndex = -1;
			ColumnBreakIndex = -1;
		}

		// Начинает располагать поля диалога внутри single box
		void StartSingleBox(int MessageId=-1, bool LeftAlign=false)
		{
			T *SingleBox = AddDialogItem(DI_SINGLEBOX, MessageId == -1 ? EMPTY_TEXT : GetLangString(MessageId));
			SingleBox->Flags = LeftAlign ? DIF_LEFTTEXT : DIF_NONE;
			SingleBox->X1 = 5;
			SingleBox->Y1 = NextY++;
			Indent = 2;
			SingleBoxIndex = DialogItemsCount - 1;
		}

		// Завершает расположение полей диалога внутри single box
		void EndSingleBox()
		{
			if (SingleBoxIndex != -1)
			{
				DialogItems[SingleBoxIndex].Y2 = NextY++;
				Indent = 0;
				SingleBoxIndex = -1;
			}
		}

		// Добавляет пустую строку.
		void AddEmptyLine()
		{
			NextY++;
		}

		// Добавляет сепаратор.
		void AddSeparator(int MessageId=-1)
		{
			T *Separator = AddDialogItem(DI_TEXT, MessageId == -1 ? EMPTY_TEXT : GetLangString(MessageId));
			Separator->Flags = DIF_SEPARATOR;
			Separator->X1 = -1;
			Separator->Y1 = Separator->Y2 = NextY++;
		}

		// Добавляет сепаратор, кнопки OK и Cancel.
		// см. также ниже AddFooterButtons
		// возвращает индекс первой добавленной кнопки
		int AddOKCancel(int OKMessageId, int CancelMessageId, bool Separator=true)
		{
			if (Separator)
				AddSeparator();

			T *OKButton = AddDialogItem(DI_BUTTON, GetLangString(OKMessageId));
			OKButton->Flags |= DIF_CENTERGROUP;
			#if FAR_UNICODE>=1906
			OKButton->Flags |= DIF_DEFAULTBUTTON;
			#else
			OKButton->DefaultButton = TRUE;
			#endif
			OKButton->Y1 = OKButton->Y2 = NextY++;
			OKButtonID = DialogItemsCount-1;

			T *CancelButton = AddDialogItem(DI_BUTTON, GetLangString(CancelMessageId));
			CancelButton->Flags = DIF_CENTERGROUP;
			CancelButton->Y1 = CancelButton->Y2 = OKButton->Y1;
			CancelButtonID = DialogItemsCount-1;
			
			return OKButtonID;
		}

		// Добавляет сепаратор, указанные кнопки, возвращает индекс первой добавленной кнопки
		int AddFooterButtons(int* BtnID, int nBtnCount)
		{
			AddSeparator();

			for (int i = 0; i < nBtnCount; i++)
			{
				T *Button = AddDialogItem(DI_BUTTON, GetLangString(BtnID[i]));
				Button->Flags |= DIF_CENTERGROUP;
				if (i == 0)
				{
					#if FAR_UNICODE>=1906
					Button->Flags |= DIF_DEFAULTBUTTON;
					#else
					Button->DefaultButton = TRUE;
					#endif
					OKButtonID = DialogItemsCount-1;
				}
				Button->Y1 = Button->Y2 = NextY;
			}
			CancelButtonID = DialogItemsCount-1;

			NextY++;
			
			return OKButtonID;
		}

		// Добавляет сепаратор, указанные кнопки, возвращает индекс первой добавленной кнопки
		int AddFooterButtons(int FirstBtn, ...)
		{
			va_list argptr;
			va_start(argptr, FirstBtn);
			int nBtns[100] = {FirstBtn}, nCount = 1, nNext;
			
			while ((nCount < ARRAYSIZE(nBtns))
				&& ((nNext = va_arg( argptr, int )) != 0))
			{
				#ifdef _DEBUG
				if (nNext>1024 || nNext<0)
				{
					_ASSERTE(nNext>0 && nNext<=1024);
					break;
				}
				#endif
				nBtns[nCount++] = nNext;
			}
			
			if (nCount > 0)
			{
				AddFooterButtons(nBtns, nCount);
				return OKButtonID;
			}
			
			return -1;
		}
		
		bool ShowDialog(int *pnBtnNo = NULL)
		{
			_ASSERTE(OKButtonID > 0 && OKButtonID < DialogItemsCount);
			UpdateBorderSize();
			UpdateSecondColumnPosition();
			UpdateRightSlidePosition();
			int Result = DoShowDialog();
			if (Result >= OKButtonID && Result < CancelButtonID)
			{
				SaveValues();
				if (pnBtnNo) *pnBtnNo = (Result - OKButtonID);
				return true;
			}
			return false;
		}
};

class PluginDialogBuilder;

class DialogAPIBinding: public DialogItemBinding<FarDialogItem>
{
protected:
	const PluginStartupInfo &Info;
	HANDLE *DialogHandle;
	int ID;

	DialogAPIBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID)
		: Info(aInfo), DialogHandle(aHandle), ID(aID)
	{
	}
};

class PluginCheckBoxBinding: public DialogAPIBinding
{
	BOOL *Value;
	int Mask;

public:
	PluginCheckBoxBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, BOOL *aValue, int aMask)
		: DialogAPIBinding(aInfo, aHandle, aID),
		  Value(aValue), Mask(aMask)
	{
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		BOOL Selected = static_cast<BOOL>(Info.SendDlgMessage(*DialogHandle, DM_GETCHECK, ID, 0));
		if (!Mask)
		{
			*Value = Selected;
		}
		else
		{
			if (Selected)
				*Value |= Mask;
			else
				*Value &= ~Mask;
		}
	}
};

class PluginRadioButtonBinding: public DialogAPIBinding
{
	private:
		int *Value;

	public:
		PluginRadioButtonBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, int *aValue)
			: DialogAPIBinding(aInfo, aHandle, aID),
			  Value(aValue)
		{
		}

		virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
		{
			if (Info.SendDlgMessage(*DialogHandle, DM_GETCHECK, ID, 0))
				*Value = RadioGroupIndex;
		}
};

class PluginComboBoxBinding: public DialogAPIBinding
{
	private:
		int *Value;

	public:
		PluginComboBoxBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, int *aValue)
			: DialogAPIBinding(aInfo, aHandle, aID),
			  Value(aValue)
		{
		}

		virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
		{
			*Value = (int)Info.SendDlgMessage(*DialogHandle, DM_LISTGETCURPOS, ID, 0);
		}
};

#ifdef UNICODE

class PluginEditFieldBinding: public DialogAPIBinding
{
private:
	TCHAR *Value;
	int MaxSize;

public:
	PluginEditFieldBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, TCHAR *aValue, int aMaxSize)
		: DialogAPIBinding(aInfo, aHandle, aID), Value(aValue), MaxSize(aMaxSize)
	{
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		const TCHAR *DataPtr = (const TCHAR *) Info.SendDlgMessage(*DialogHandle, DM_GETCONSTTEXTPTR, ID, 0);
		lstrcpyn(Value, DataPtr, MaxSize);
	}
};

class PluginIntEditFieldBinding: public DialogAPIBinding
{
private:
	int *Value;
	TCHAR Buffer[32];
	TCHAR Mask[32];

public:
	PluginIntEditFieldBinding(const PluginStartupInfo &aInfo, HANDLE *aHandle, int aID, int *aValue, int Width)
		: DialogAPIBinding(aInfo, aHandle, aID),
		  Value(aValue)
	{
		aInfo.FSF->sprintf(Buffer, L"%u", *aValue);
		int MaskWidth = Width < 31 ? Width : 31;
		for(int i=0; i<MaskWidth; i++)
			Mask[i] = '9';
		Mask[MaskWidth] = '\0';
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		const TCHAR *DataPtr = (const TCHAR *) Info.SendDlgMessage(*DialogHandle, DM_GETCONSTTEXTPTR, ID, 0);
		*Value = Info.FSF->atoi(DataPtr);
	}

	TCHAR *GetBuffer()
	{
		return Buffer;
	}

	const TCHAR *GetMask()
	{
		return Mask;
	}
};

#else

class PluginEditFieldBinding: public DialogItemBinding<FarDialogItem>
{
private:
	TCHAR *Value;
	int MaxSize;

public:
	PluginEditFieldBinding(TCHAR *aValue, int aMaxSize)
		: Value(aValue), MaxSize(aMaxSize)
	{
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		lstrcpyn(Value, Item->Data, MaxSize);
	}
};

class PluginIntEditFieldBinding: public DialogItemBinding<FarDialogItem>
{
private:
	const PluginStartupInfo &Info;
	int *Value;
	TCHAR Mask[32];

public:
	PluginIntEditFieldBinding(const PluginStartupInfo &aInfo, int *aValue, int Width)
		: Info(aInfo), Value(aValue)
	{
		int MaskWidth = Width < 31 ? Width : 31;
		for(int i=0; i<MaskWidth; i++)
			Mask[i] = '9';
		Mask[MaskWidth] = '\0';
	}

	virtual void SaveValue(FarDialogItem *Item, int RadioGroupIndex)
	{
		*Value = Info.FSF->atoi(Item->Data);
	}

	const TCHAR *GetMask()
	{
		return Mask;
	}
};

#endif

/*
Версия класса для динамического построения диалогов, используемая в плагинах к Far.
*/
class PluginDialogBuilder: public DialogBuilderBase<FarDialogItem>
{
	public:
		DWORD DialogFlags;
	protected:
		const PluginStartupInfo &Info;
		HANDLE DialogHandle;
		const TCHAR *HelpTopic;
	private:
		static const void* gpDialogBuilderPtr;
	protected:

		virtual void InitDialogItem(FarDialogItem *Item, const TCHAR *Text)
		{
			memset(Item, 0, sizeof(FarDialogItem));
#ifdef UNICODE
	#if FAR_UNICODE>=1962
			Item->Data = Text;
	#else
			Item->PtrData = Text;
	#endif
#else
			lstrcpyn(Item->Data, Text, sizeof(Item->Data)/sizeof(Item->Data[0]));
#endif
		}

		virtual int TextWidth(const FarDialogItem &Item)
		{
#ifdef UNICODE
	#if FAR_UNICODE>=1962
			return lstrlen(Item.Data);
	#else
			return lstrlen(Item.PtrData);
	#endif
#else
			return lstrlen(Item.Data);
#endif
		}

		virtual const TCHAR *GetLangString(int MessageID)
		{
			LPCTSTR pszMsg = NULL;
			#if FAR_UNICODE>=1906
			pszMsg = Info.GetMsg(&guid_PluginGuid, MessageID);
			#else
			pszMsg = Info.GetMsg(Info.ModuleNumber, MessageID);
			#endif
			return pszMsg;
		}
		
		virtual BOOL PluginDialogProc(HANDLE hDlg, int Msg, int Param1, FarDlgProcParam2 Param2, LONG_PTR& lResult)
		{
			#ifdef _UNICODE
			if (Msg == DM_GETDIALOGINFO)
			{
				DialogInfo* pId = (DialogInfo*)Param2;
				_ASSERTE(pId->StructSize == sizeof(DialogInfo));
				pId->Id = DlgGuid;
				lResult = TRUE;
				return TRUE;
			}
			#endif
			// Если возвращается FALSE - вызовется DefDlgProc(hDlg, Msg, Param1, Param2)
			return FALSE;
		}

		#if FAR_UNICODE>=3000
		static intptr_t WINAPI PluginDialogBuilderStaticProc(HANDLE hDlg, intptr_t Msg, intptr_t Param1, void* Param2)
		#elif FAR_UNICODE>=1988
		static INT_PTR WINAPI PluginDialogBuilderStaticProc(HANDLE hDlg, int Msg, int Param1, void* Param2)
		#else
		static LONG_PTR WINAPI PluginDialogBuilderStaticProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
		#endif
		{
			LONG_PTR lRc = 0;
			PluginDialogBuilder* pDlg = (PluginDialogBuilder*)((PluginStartupInfo*)gpDialogBuilderPtr)->SendDlgMessage(hDlg, DM_GETDLGDATA, 0, 0);
			BOOL lbProcessed = FALSE;
			if (pDlg)
				lbProcessed = pDlg->PluginDialogProc(hDlg, Msg, Param1, Param2, lRc);
			if (!lbProcessed)
				lRc = ((PluginStartupInfo*)gpDialogBuilderPtr)->DefDlgProc(hDlg, Msg, Param1, Param2);
			return lRc;
		}
		
		virtual int DoShowDialog()
		{
			int iRc = -1;
			int Width = DialogItems [0].X2+4;
			int Height = DialogItems [0].Y2+2;
			
			#ifdef UNICODE
				#if FAR_UNICODE>=1988
					DialogHandle = Info.DialogInit(&guid_PluginGuid, &DlgGuid,
											 -1, -1, Width, Height,
											 HelpTopic, DialogItems, DialogItemsCount, 
											 0, DialogFlags, PluginDialogBuilderStaticProc, (FarDlgProcParam2)this);
				#else
					DialogHandle = Info.DialogInit(Info.ModuleNumber, -1, -1, Width, Height,
						HelpTopic, DialogItems, DialogItemsCount, 0, DialogFlags, PluginDialogBuilderStaticProc, (LONG_PTR)this);
				#endif
				iRc = Info.DialogRun(DialogHandle);
			#else
				iRc = Info.DialogEx(Info.ModuleNumber, -1, -1, Width, Height,
					HelpTopic, DialogItems, DialogItemsCount, 0, DialogFlags, PluginDialogBuilderStaticProc, (LONG_PTR)this);
			#endif
			
			return iRc;
		}

		virtual DialogItemBinding<FarDialogItem> *CreateCheckBoxBinding(BOOL *Value, int Mask)
		{
#ifdef UNICODE
			return new PluginCheckBoxBinding(Info, &DialogHandle, DialogItemsCount-1, Value, Mask);
#else
			return new CheckBoxBinding<FarDialogItem>(Value, Mask);
#endif
		}

		virtual DialogItemBinding<FarDialogItem> *CreateRadioButtonBinding(BOOL *Value)
		{
#ifdef UNICODE
			return new PluginRadioButtonBinding(Info, &DialogHandle, DialogItemsCount-1, Value);
#else
			return new RadioButtonBinding<FarDialogItem>(Value);
#endif
		}

		virtual DialogItemBinding<FarDialogItem>* CreateComboBoxBinding(int *Value)
		{
#ifdef UNICODE
			return new PluginComboBoxBinding(Info, &DialogHandle, DialogItemsCount-1, Value);
#else
			return new PluginComboBoxBinding<FarDialogItem>(Value);
#endif
		}

public:
		PluginDialogBuilder(const PluginStartupInfo &aInfo, int TitleMessageID, const TCHAR *aHelpTopic, const GUID* aDlgGuid)
			: DialogBuilderBase(aDlgGuid), DialogFlags(0), Info(aInfo), DialogHandle(NULL), HelpTopic(aHelpTopic)
		{
			gpDialogBuilderPtr = (LPVOID*)&aInfo;
			AddBorder(GetLangString(TitleMessageID));
		}

		virtual ~PluginDialogBuilder()
		{
#ifdef UNICODE
			if (DialogHandle)
			{
				Info.DialogFree(DialogHandle);
				DialogHandle = NULL;
			}
#endif
		}

		FarDialogItem *GetItemByIndex(int Index)
		{
			if (Index >= 0 && Index < DialogItemsCount)
				return (DialogItems + Index);
			return NULL;
		}

		int GetItemIndex(FarDialogItem* p)
		{
			return GetItemID(p);
		}

		virtual FarDialogItem *AddIntEditField(int *Value, int Width)
		{
			FarDialogItem *Item = AddDialogItem(DI_FIXEDIT, EMPTY_TEXT);
			Item->Flags |= DIF_MASKEDIT;
			PluginIntEditFieldBinding *Binding;
#ifdef UNICODE
			Binding = new PluginIntEditFieldBinding(Info, &DialogHandle, DialogItemsCount-1, Value, Width);
	#if FAR_UNICODE>=1962
			Item->Data = Binding->GetBuffer();
	#else
			Item->PtrData = Binding->GetBuffer();
	#endif
#else
			Binding = new PluginIntEditFieldBinding(Info, Value, Width);
			Info.FSF->itoa(*Value, (TCHAR *) Item->Data, 10);
#endif


#ifdef _FAR_NO_NAMELESS_UNIONS
			Item->Param.Mask = Binding->GetMask();
#else
			Item->Mask = Binding->GetMask();
#endif
			SetNextY(Item);
			Item->X2 = Item->X1 + Width - 1;
			SetLastItemBinding(Binding);
			return Item;
		}

		FarDialogItem *AddEditField(TCHAR *Value, int MaxSize, int Width, const TCHAR *HistoryID = nullptr)
		{
			FarDialogItem *Item = AddDialogItem(DI_EDIT, Value);
			SetNextY(Item);
			Item->X2 = Item->X1 + Width;
			if (HistoryID)
			{
#ifdef _FAR_NO_NAMELESS_UNIONS
				Item->Param.History = HistoryID;
#else
				Item->History = HistoryID;
#endif
				Item->Flags |= DIF_HISTORY;
			}

#ifdef UNICODE
			SetLastItemBinding(new PluginEditFieldBinding(Info, &DialogHandle, DialogItemsCount-1, Value, MaxSize));
#else
			SetLastItemBinding(new PluginEditFieldBinding(Value, MaxSize));
#endif
			return Item;
		}
		
		void InitFocus(FarDialogItem* p)
		{
			#if FAR_UNICODE>=1906
			p->Flags |= DIF_FOCUS;
			#else
			p->Focus = TRUE;
			#endif
		}
};
