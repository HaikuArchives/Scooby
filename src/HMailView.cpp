#include "HMailView.h"
#include "HApp.h"
#include "HPrefs.h"
#include "Encoding.h"
#include "HMailList.h"
#include "HWindow.h"
#include "base64.h"

#include <Menu.h>
#include <Font.h>
#include <List.h>
#include <MenuItem.h>
#include <E-mail.h>
#include <fs_attr.h>
#include <stdlib.h>
#include <stdio.h>
#include <Roster.h>
#include <Beep.h>
#include <Alert.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <File.h>
#include <Application.h>
#include <string.h>
#include <ctype.h>
#include <Path.h>
#include <Debug.h>
#include <Autolock.h>
#include <FindDirectory.h>

const rgb_color kQuote1 = {0,150,0};
const rgb_color kQuote2 = {150,150,0};
const rgb_color kQuote3 = {0,150,150};
const rgb_color kBlack = {0,0,0};

HMailView::HMailView(BRect frame, bool incoming, BFile *file)
		  :_inherited(frame, "HMailView",
		  	 B_FOLLOW_ALL, B_WILL_DRAW |B_NAVIGABLE)
{
	BFont	font = *be_plain_font;

	fIncoming = incoming;
	fFile = file;
	fReady = false;
	fLastPosition = -1;
	fYankBuffer = NULL;
	fStopSem = create_sem(1, "reader_sem");
	fThread = 0;
	fHeader = false;
	fRaw = false;
	SetStylable(true);
	SetWordWrap(true);
	fEnclosures = new BList();
	fPanel = NULL;

	font.SetSize(10);
	fMenu = new BPopUpMenu("Attachments", FALSE, FALSE);
	fMenu->SetFont(&font);
	BString label = _("Save Attachment");
	label += B_UTF8_ELLIPSIS;
	fMenu->AddItem(new BMenuItem(label.String(), new BMessage(M_SAVE)));
	fMenu->AddItem(new BMenuItem(_("Open Attachment"), new BMessage(M_ADD)));
	ResetFont();
}

//--------------------------------------------------------------------

HMailView::~HMailView(void)
{
	ClearList();
	if (fPanel)
		delete fPanel;
	if (fYankBuffer)
		delete[] fYankBuffer;
	delete_sem(fStopSem);
	delete fFile;
	delete fMenu;
}

//--------------------------------------------------------------------

void HMailView::AttachedToWindow(void)
{
	_inherited::AttachedToWindow();
	if (fFile) {
		LoadMessage(fFile, FALSE, FALSE, NULL);
		if (fIncoming)
			MakeEditable(FALSE);
	}
}

void
HMailView::ResetFont()
{
	HPrefs *prefs = ((HApp*)be_app)->Prefs();
	const char* family,*style;
	float size;

	prefs->GetData("font_family",&family);
	prefs->GetData("font_style",&style);
	prefs->GetData("font_size",&size);

	fFont.SetFamilyAndStyle(family,style);
	fFont.SetSize(size);

	fFont.SetSpacing(B_FIXED_SPACING);
	SetFontAndColor(0,TextLength(),&fFont);
}

//--------------------------------------------------------------------

void HMailView::KeyDown(const char *key, int32 count)
{
	bool	up = FALSE;
	char	new_line = '\n';
	int32	end;
	int32 	start;
	uint32	mods;

	mods = Window()->CurrentMessage()->FindInt32("modifiers");

	switch (key[0]) {
		case B_LEFT_ARROW:	// Select prev or next word.
		case B_RIGHT_ARROW:
			if(mods & B_CONTROL_KEY)
			{
				GetSelection(&start,&end);
				FindWord((key[0] == B_LEFT_ARROW)?start-2:end+1,&start,&end);
				Select(start,end);
			}else
				_inherited::KeyDown(key,count);
			break;
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			if (IsEditable())
				_inherited::KeyDown(key, count);
			else {
				if ((Bounds().top) && (key[0] == B_UP_ARROW))
					ScrollBy(0, LineHeight() * -1);
				else if ((Bounds().bottom < CountLines() * LineHeight()) &&
						 (key[0] == B_DOWN_ARROW))
					ScrollBy(0, LineHeight());
			}
			break;

		case B_HOME:
			if (IsSelectable()) {
				if (mods & B_CONTROL_KEY)	// ^a - start of doc **GR
				{
					Select(0, 0);
					ScrollToSelection();
				}
				else
				{
					GoToLine(CurrentLine());
				}
			}
			break;

		case 0x02:						// ^b - back 1 char
			if (IsSelectable()) {
				GetSelection(&start, &end);
				start--;
				if (start >= 0) {
					Select(start, start);
					ScrollToSelection();
				}
			}
			break;
		case B_END:
		{
			if (IsSelectable())
			{
				if (mods & B_CONTROL_KEY)	// **GR
					Select(TextLength(), TextLength());
				else
				{
					int32 eol = OffsetAt(CurrentLine()+1) - 1;
					if (eol == TextLength() - 1) eol++;	// special case at EOF

					Select(eol, eol);
				}
				ScrollToSelection();
			}
			break;
		}
		case 0x05:						// ^e - end of line
			if ((IsSelectable()) && (mods & B_CONTROL_KEY)) {
				GoToLine(CurrentLine() + 1);
				GetSelection(&start, &end);
				Select(start - 1, start - 1);
			}
			break;

		case 0x06:						// ^f - forward 1 char
			if (IsSelectable()) {
				GetSelection(&start, &end);
				if (end > start)
					start = end;
				Select(start + 1, start + 1);
				ScrollToSelection();
			}
			break;

		case 0x0e:						// ^n - next line
			if (IsSelectable()) {
				GoToLine(CurrentLine() + 1);
				ScrollToSelection();
			}
			break;

		case 0x0f:						// ^o - open line
			if (IsEditable()) {
				GetSelection(&start, &end);
				Delete();
				Insert(&new_line, 1);
				Select(start, start);
				ScrollToSelection();
			}
			break;

		case B_PAGE_UP:
			if (mods & B_CONTROL_KEY) {	// ^k kill text from cursor to e-o-line
				if (IsEditable()) {
					GetSelection(&start, &end);
					if ((start != fLastPosition) && (fYankBuffer)) {
						delete[] fYankBuffer;
						fYankBuffer = NULL;
					}
					fLastPosition = start;
					if (CurrentLine() < (CountLines() - 1)) {
						GoToLine(CurrentLine() + 1);
						GetSelection(&end, &end);
						end--;
					}
					else
						end = TextLength();
					if (end < start)
						break;
					if (start == end)
						end++;
					Select(start, end);
					if (fYankBuffer) {
						fYankBuffer = (char *)realloc(fYankBuffer,
									 strlen(fYankBuffer) + (end - start) + 1);
						GetText(start, end - start,
							    &fYankBuffer[strlen(fYankBuffer)]);
					}
					else {
						fYankBuffer = new char[end - start + 1];
						GetText(start, end - start, fYankBuffer);
					}
					Delete();
					ScrollToSelection();
				}
				break;
			}
			else
				up = TRUE;
				_inherited::KeyDown(key,count);
			 break;
				// yes, fall through!
		/*
		case B_PAGE_DOWN:
			r = Bounds();
			height = (int32)(up ? r.top - r.bottom : r.bottom - r.top) - 25;
			if ((up) && (!r.top))
				break;
			else if ((!up) && (r.bottom >= CountLines() * LineHeight()))
				break;
			ScrollBy(0, height);
			break;
		*/
		case 0x10:						// ^p goto previous line
			if (IsSelectable()) {
				GoToLine(CurrentLine() - 1);
				ScrollToSelection();
			}
			break;

		case 0x19:						// ^y yank text
			if ((IsEditable()) && (fYankBuffer)) {
				Delete();
				Insert(fYankBuffer);
				ScrollToSelection();
			}
			break;
		default:
			_inherited::KeyDown(key, count);
	}
}

//--------------------------------------------------------------------

void HMailView::MakeFocus(bool focus)
{
	_inherited::MakeFocus(focus);
}

//--------------------------------------------------------------------

void HMailView::MessageReceived(BMessage *msg)
{
	bool		inserted = FALSE;
	bool		enclosure = FALSE;
	char		type[B_FILE_NAME_LENGTH];
	char		*text;
	int32		end;
	int32		index = 0;
	int32		loop;
	int32		offset;
	int32		start;
	BFile		file;
	BMessage	message(REFS_RECEIVED);
	BNodeInfo	*node;
	entry_ref	ref;
	off_t		len = 0;
	off_t		size;

	switch (msg->what) {
		case B_SIMPLE_DATA:
			if (!fIncoming) {
				while (msg->FindRef("refs", index++, &ref) == B_NO_ERROR) {
					file.SetTo(&ref, O_RDONLY);
					if (file.InitCheck() == B_NO_ERROR) {
						node = new BNodeInfo(&file);
						node->GetType(type);
						delete node;
						file.GetSize(&size);
						if ((!strncmp(type, "text/", 5)) && (size)) {
							len += size;
							text = new char[size];
							file.Read(text, size);
							if (!inserted) {
								GetSelection(&start, &end);
								Delete();
								inserted = TRUE;
							}
							offset = 0;
							for (loop = 0; loop < size; loop++) {
								if (text[loop] == '\n') {
									Insert(&text[offset], loop - offset + 1);
									offset = loop + 1;
								}
								else if (text[loop] == '\r') {
									text[loop] = '\n';
									Insert(&text[offset], loop - offset + 1);
									if ((loop + 1 < size) && (text[loop + 1] == '\n'))
										loop++;
									offset = loop + 1;
								}
							}
							delete[] text;
						}
						else {
							enclosure = TRUE;
							message.AddRef("refs", &ref);
						}
					}
				}
				if (inserted)
					Select(start, start + len);
				if (enclosure)
					Window()->PostMessage(&message, Window());
			}
			break;

		case M_HEADER:
			UnlockLooper();
			StopLoad();
			LockLooper();
			msg->FindBool("header", &fHeader);
			ResetTextRunArray();
			SetText(NULL);
			LoadMessage(fFile, FALSE, FALSE, NULL);
			break;

		case M_RAW:
			UnlockLooper();
			StopLoad();
			LockLooper();
			msg->FindBool("raw", &fRaw);
			ResetTextRunArray();
			SetText(NULL);
			LoadMessage(fFile, FALSE, FALSE, NULL);
			break;

		case M_SELECT:
			if (IsSelectable())
				Select(0, TextLength());
			break;

		case M_SAVE:
			Save(msg);
			break;
		case M_SET_CONTENT:
		{
			BFile *file;
			if(msg->FindPointer("pointer",(void**)&file) == B_OK)
				SetContent(file);
			else
				SetContent(NULL);
			break;
		}
		case 'find':
		{
			const char* text;
			if(msg->FindString("findthis",&text) == B_OK)
				Find(text);
			break;
		}
		// DND reply message from Tracker.
		case B_COPY_TARGET:
		{
			entry_ref dirRef;
			const char* name;
			BMessage dataMsg;
			if(msg->FindString("name",&name) == B_OK &&
				msg->FindMessage("be:originator-data",&dataMsg) == B_OK &&
				msg->FindRef("directory",&dirRef) == B_OK)
			{
				BPath path(&dirRef);
				PRINT(("%s\n",path.Path()));
				path.Append(name);
				BFile file(path.Path(),B_WRITE_ONLY);
				if(file.InitCheck() != B_OK)
					break;
				const char* data;
				ssize_t size;
				dataMsg.FindData("data",B_ANY_TYPE,(const void**)&data,&size);
				file.Write(data,size);
				file.SetSize(size);
			}
			break;
		}
		default:
			_inherited::MessageReceived(msg);
	}
}

/***********************************************************
 * ResetTextRunArray
 ***********************************************************/
void HMailView::ResetTextRunArray()
{
	text_run_array array;
	text_run	run;

	run.offset = 0;
	run.font = fFont;
	run.color = kBlack;
	array.runs[0] = run;
	array.count = 1;
	SetRunArray(0,TextLength()-1,&array);
}

//--------------------------------------------------------------------

void HMailView::MouseDown(BPoint where)
{
	int32			items;
	int32			loop;
	int32			start;
	uint32			buttons;
	BMenuItem		*item;
	BPoint			point;
	bigtime_t		click;
	hyper_text		*enclosure;

	if (Window()->CurrentMessage())
		Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
	if(buttons == B_PRIMARY_MOUSE_BUTTON)
	{
		start = OffsetAt(where);
		items = fEnclosures->CountItems();
		for (loop = 0; loop < items; loop++) {
			enclosure = (hyper_text *)fEnclosures->ItemAt(loop);
			if ((start >= enclosure->text_start) && (start < enclosure->text_end)) {
				Select(enclosure->text_start, enclosure->text_end);
				Window()->UpdateIfNeeded();
				get_click_speed(&click);
				click += system_time();
				point = where;
				while ((buttons) && (abs((int)(point.x - where.x)) < 4) &&
					   (abs((int)(point.y - where.y)) < 4) &&
					    (system_time() < click))
				{
					snooze(10000);
					GetMouse(&point, &buttons);
				}
				if (system_time() >= click)
				{
					if ((enclosure->type != TYPE_ENCLOSURE) &&
						(enclosure->type != TYPE_BE_ENCLOSURE))
					return;
					ConvertToScreen(&point);
					item = fMenu->Go(point, TRUE);
					if (item)
					{
						if (item->Message()&&item->Message()->what == M_SAVE)
						{
							if (fPanel)
								fPanel->SetEnclosure(enclosure);
							else{
								fPanel = new TSavePanel(enclosure, this);
								fPanel->Window()->Show();
							}
						return;
						}
					}
					else
						return;
				}
				else if ((abs( (int)(point.x - where.x) ) < 4) &&
						 (abs((int)(point.y - where.y)) < 4))
					Open(enclosure);
				else
					_inherited::MouseDown(where);
				return;
			}
		}
		_inherited::MouseDown(where);
	}else if(buttons == B_SECONDARY_MOUSE_BUTTON){
		if(!fIncoming)
			return _inherited::MouseDown(where);

		BPopUpMenu* theMenu = new BPopUpMenu("Attachments");
		BFont font(be_plain_font);
		font.SetSize(10);
		theMenu->SetFont(&font);
		BMessage *msg =  new BMessage(M_HEADER);
		msg->AddBool("header",!fHeader);
		theMenu->AddItem((item = new BMenuItem(_("Show Headers"),msg,'H',0)));
		item->SetMarked(fHeader);
		item->SetEnabled( (fFile)?true:false);
		msg = new BMessage(M_RAW);
		msg->AddBool("raw",!fRaw);
		theMenu->AddItem((item= new BMenuItem(_("Show Raw Message"), msg)));
		item->SetMarked(fRaw);
		item->SetEnabled( (fFile)?true:false);

		theMenu->AddSeparatorItem();
		msg = new BMessage(M_PRINT_MESSAGE);
		msg->AddPointer("view",this);
		msg->AddString("job_name","untitled");
		theMenu->AddItem((item = new BMenuItem(_("Print"),msg,'P',0)));
		item->SetEnabled( (TextLength() > 0)?true:false);

		start = OffsetAt(where);
		theMenu->AddSeparatorItem();
		BMenuItem *saveItem,*openItem;
		BString label = _("Save Attachment");
		label += B_UTF8_ELLIPSIS;
		theMenu->AddItem( saveItem = new BMenuItem(label.String(), new BMessage(M_SAVE)));
		theMenu->AddItem( openItem = new BMenuItem(_("Open Attachment"), new BMessage(M_ADD)));
		openItem->SetEnabled(false);
		saveItem->SetEnabled(false);
		enclosure = NULL;

		items = fEnclosures->CountItems();
		for (loop = 0; loop < items; loop++)
		{
			enclosure = (hyper_text *)fEnclosures->ItemAt(loop);
			if ((start >= enclosure->text_start) && (start < enclosure->text_end))
			{
				if ((enclosure->type == TYPE_ENCLOSURE) ||
						(enclosure->type == TYPE_BE_ENCLOSURE))
				{
					saveItem->SetEnabled(true);
					openItem->SetEnabled(true);
					break;
				}
			}
		}

		BRect r;
        ConvertToScreen(&where);
        r.top = where.y - 5;
        r.bottom = where.y + 5;
        r.left = where.x - 5;
        r.right = where.x + 5;

    	BMenuItem *theItem = theMenu->Go(where, false,true,r);
    	if(theItem)
    	{
    	 	BMessage*	aMessage = theItem->Message();
			if(aMessage->what == M_SAVE )
			{
				if (fPanel)
					fPanel->SetEnclosure(enclosure);
				else{
					fPanel = new TSavePanel(enclosure, this);
					fPanel->Window()->Show();
				}
			}else if(aMessage->what == M_ADD){
				Open(enclosure);
			}else
	 			Window()->PostMessage(aMessage);
	 	}
	 	delete theMenu;
	}else
		_inherited::MouseDown(where);
}

//--------------------------------------------------------------------

void HMailView::MouseMoved(BPoint where, uint32 code, const BMessage *msg)
{
	int32			items;
	int32			loop;
	int32			start;
	hyper_text		*enclosure;

	start = OffsetAt(where);
	items = fEnclosures->CountItems();
	for (loop = 0; loop < items; loop++) {
		enclosure = (hyper_text *)fEnclosures->ItemAt(loop);
		if ((start >= enclosure->text_start) && (start < enclosure->text_end)) {
			be_app->SetCursor(B_HAND_CURSOR);
			return;
		}
	}
	_inherited::MouseMoved(where, code, msg);
}

//--------------------------------------------------------------------

void HMailView::ClearList(void)
{
	BEntry			entry;
	hyper_text		*enclosure;

	while ((enclosure = (hyper_text *)fEnclosures->FirstItem())) {
		fEnclosures->RemoveItem(enclosure);
		if (enclosure->name)
			delete[] enclosure->name;
		if (enclosure->content_type)
			delete[] enclosure->content_type;
		if (enclosure->encoding)
			delete[] enclosure->encoding;
		if ((enclosure->have_ref) && (!enclosure->saved)) {
			entry.SetTo(&enclosure->ref);
			entry.Remove();
		}
		delete enclosure;
	}
}

void HMailView::SetContent(BFile *file)
{
	UnlockLooper();
	StopLoad();
	LockLooper();
	SetText("");
	delete fFile;
	fFile = NULL;
	if(file)
		LoadMessage(file,false,false,NULL);
}

//--------------------------------------------------------------------

void HMailView::LoadMessage(BFile *file, bool quote_it, bool close,
							const char *text)
{
	reader			*info;
	attr_info		a_info;

	ClearList();
	MakeSelectable(FALSE);
	if (text)
		Insert(text, strlen(text));
	fFile = file;
	info = new reader;
	info->header = fHeader;
	info->raw = fRaw;
	info->quote = quote_it;
	info->incoming = fIncoming;
	info->close = close;
	if (file->GetAttrInfo(B_MAIL_ATTR_MIME, &a_info) == B_NO_ERROR)
		info->mime = TRUE;
	else
		info->mime = FALSE;
	info->view = this;
	info->enclosures = fEnclosures;
	info->file = file;
	info->stop_sem = &fStopSem;
	resume_thread(fThread = spawn_thread((status_t (*)(void *)) Reader,
							   "reader", B_DISPLAY_PRIORITY, info));
}

//--------------------------------------------------------------------

void HMailView::Open(hyper_text *enclosure)
{
	char		name[B_FILE_NAME_LENGTH];
	char		name1[B_FILE_NAME_LENGTH];
	int32		index = 0;
	BDirectory	dir;
	BEntry		entry;
	BMessage	save(M_SAVE);
	BMessage	open(B_REFS_RECEIVED);
	BPath		path;
	BMessenger	*tracker;
	entry_ref	ref;
	status_t	result;
	switch (enclosure->type) {
		case TYPE_HTML:
		case TYPE_FTP:
			result = be_roster->Launch("text/html", 1, &enclosure->name);
			if ((result != B_NO_ERROR) && (result != B_ALREADY_RUNNING)) {
				beep();
				(new BAlert("", _("There is no installed handler for 'text/html'."),
					_("OK")))->Go();
			}
			break;

		case TYPE_MAILTO:
			be_roster->Launch(B_MAIL_TYPE, 1, &enclosure->name);
			break;

		case TYPE_ENCLOSURE:
		case TYPE_BE_ENCLOSURE:
			if (!enclosure->have_ref) {
				if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &path) == B_NO_ERROR) {
					dir.SetTo(path.Path());
					if (dir.InitCheck() == B_NO_ERROR) {
						if (enclosure->name)
						{
							BString decodedName(enclosure->name);
							Encoding().Mime2UTF8(decodedName);
							sprintf(name1, "%s", decodedName.String());
						}else{
							sprintf(name1, _("Untitled"));
						}
						strcpy(name, name1);
						while (dir.Contains(name)) {
							sprintf(name, "%s_%d", name1, (int)index++);
						}
						entry.SetTo(path.Path());
						entry.GetRef(&ref);
						save.AddRef("directory", &ref);
						save.AddString("name", name);
						save.AddPointer("enclosure", enclosure);
						if (Save(&save) != B_NO_ERROR)
							break;
						enclosure->saved = FALSE;
					}
				}
			}
			tracker = new BMessenger("application/x-vnd.Be-TRAK", -1, NULL);
			if (tracker->IsValid()) {
				open.AddRef("refs", &enclosure->ref);
				tracker->SendMessage(&open);
			}
			delete tracker;
			//be_roster->Launch(&enclosure->ref);
			break;
	}
}

//--------------------------------------------------------------------

status_t HMailView::Reader(reader *info)
{
	char		*msg;
	int32		len;
	off_t		size;

	info->file->GetSize(&size);
	if ((msg = new char[size]) == NULL)
		goto done;
	info->file->Seek(0, 0);
	size = info->file->Read(msg, size);

	info->file->ReadAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &len, sizeof(int32));
	if ((info->header) && (len)) {
		if (!strip_it(msg, len, info))
			goto done;
	}

	if ((info->raw) || (!info->mime)) {
		if (!strip_it(msg + len, size - len, info))
			goto done;
	}
	else if (!parse_header(msg, msg, size, NULL, info, NULL))
		goto done;

	if (get_semaphore(info->view->Window(), info->stop_sem)) {

		info->view->Select(0, 0);
		info->view->MakeSelectable(TRUE);
		if (!info->incoming)
			info->view->MakeEditable(TRUE);
		info->view->Window()->Unlock();
		release_sem(*(info->stop_sem));
	}

done:;
	if(info->incoming)
		HighlightQuote(info->view);
	if (info->close)
		delete info->file;
	delete info;
	if (msg)
		delete[] msg;
	return B_NO_ERROR;
}

void
HMailView::HighlightQuote(BTextView *view)
{
	BAutolock lock(view->Window());
	BString text = view->Text();
	int32 length = text.Length();

	BString line("");
	int32 i = 0,e,j;
	text_run_array *array;
	while(i < length)
	{
		e = text.FindFirst("\n",i);
		if(e == B_ERROR)
			e = length-1;
		if(i == e )
		{
			i = e+1;
			continue;
		}
		text.CopyInto(line,i,e);
		j = 0;

		if(line[0] == '>')
		{
			j++;
			if(line[1] == '>')
			{
				j++;
				if(line[2] == '>')
					j++;
			}
		}else{
			i = e+1;
			continue;
		}

		rgb_color col = kBlack;

		if(j==1)
			col = kQuote1;
		else if(j==2)
			col = kQuote2;
		else if(j==3)
			col = kQuote3;
		//PRINT(("Start: %d End:%d Col:%d\n",i,e,j));
		//view->SetFontAndColor(i,e,&font,B_FONT_ALL,&col);
		array = view->RunArray(i,e);
		int32 count = array->count;
		for(int32 m = 0;m < count;m++)
		{
			if(array->runs[m].color.red == 0&&
				array->runs[m].color.green == 0&&
				array->runs[m].color.blue == 0)
				array->runs[m].color = col;
		}
		view->SetRunArray(i,e,array);
		free(array);
		line = "";
		i = e+1;
	}
}

//--------------------------------------------------------------------

status_t HMailView::Save(BMessage *msg)
{
	bool			is_text = FALSE;
	const char		*name;
	char			*data;
	entry_ref		ref;
	BDirectory		dir;
	BEntry			entry;
	BFile			file;
	hyper_text		*enclosure;
	ssize_t			size;
	status_t		result = B_NO_ERROR;
	char 			entry_name[B_FILE_NAME_LENGTH];
	msg->FindString("name", &name);
	msg->FindRef("directory", &ref);
	msg->FindPointer("enclosure",(void**) &enclosure);
	dir.SetTo(&ref);
	if (dir.InitCheck() == B_NO_ERROR) {
		if (dir.FindEntry(name, &entry) == B_NO_ERROR)
			entry.Remove();
		if ((enclosure->have_ref) && (!enclosure->saved)) {
			entry.SetTo(&enclosure->ref);

			/* Added true arg and entry_name so MoveTo clobbers as before.
			 * This may not be the correct behaviour, but
			 * it's the preserved behaviour.
			 */
			entry.GetName(entry_name);
			if (entry.MoveTo(&dir, entry_name, true) == B_NO_ERROR) {
				entry.Rename(name);
				entry.GetRef(&enclosure->ref);
				enclosure->saved = TRUE;
				return result;
			}
		}

		if ((result = dir.CreateFile(name, &file)) == B_NO_ERROR) {
			data = new char[enclosure->file_length+1];
			fFile->Seek(enclosure->file_offset, 0);
			size = fFile->Read(data, enclosure->file_length);
			data[enclosure->file_length] = '\0';

			if (enclosure->type == TYPE_BE_ENCLOSURE)
				SaveBeFile(&file, data, size);
			else {
				if ((enclosure->encoding) && (cistrstr(enclosure->encoding, "base64"))) {
					is_text = ((cistrstr(enclosure->content_type, "text")) &&
							  (!cistrstr(enclosure->content_type, B_MAIL_TYPE)));
					size = decode64(data, data, size);
					PRINT(("MIME-B\n"));
				}else if ((enclosure->encoding) && (cistrstr(enclosure->encoding, "quoted-printable"))) {
					PRINT(("MIME-Q\n"));
					size = Encoding::decode_quoted_printable(data, data, size, false);
				}
				file.Write(data, size);
				file.SetSize(size);
			}
			delete[] data;
			// Update mime info
			BPath filePath(&ref);
			filePath.Append(name);
			::update_mime_info(filePath.Path(),false,true,true);
			//
			PRINT(("Name:%s Type:%s\n",name,enclosure->content_type));
			// Unset execute bits for security
			mode_t perm;
			if(file.GetPermissions(&perm) == B_OK)
			{
				perm &= ~S_IXUSR;
				perm &= ~S_IXGRP;
				perm &= ~S_IXOTH;
				file.SetPermissions(perm);
			}
			//
			dir.FindEntry(name, &entry);
			entry.GetRef(&enclosure->ref);
			enclosure->have_ref = true;
			enclosure->saved = true;
		}
		else {
			beep();
			(new BAlert("", _("An error occurred trying to save the attachment."),
				_("OK")))->Go();
		}
	}
	return result;
}

//--------------------------------------------------------------------

void HMailView::SaveBeFile(BFile *file, char *data, ssize_t size)
{
	char		*boundary;
	char		*encoding;
	char		*name;
	char		*offset;
	char		*start;
	char		*type = NULL;
	int32		len;
	int32		index;
	BNodeInfo	*info;
	type_code	code;
	off_t		length;

	offset = data;
	len = linelen(offset, (data + size) - offset, TRUE);
	if (len < 2)
		return;
	boundary = data;
	boundary[len - 2] = 0;

	while (1) {
		if ((!(offset = find_boundary(offset, boundary, (data + size) - offset))) ||
			(offset[strlen(boundary) + 1] == '-'))
			return;

		while ((len = linelen(offset, (data + size) - offset, TRUE)) > 2) {
			if (!cistrncmp(offset, CONTENT_TYPE, strlen(CONTENT_TYPE))) {
				offset[len - 2] = 0;
				type = offset;
			}
			else if (!cistrncmp(offset, CONTENT_ENCODING, strlen(CONTENT_ENCODING))) {
				offset[len - 2] = 0;
				encoding = offset + strlen(CONTENT_ENCODING);
			}
			offset += len;
			if (*offset == '\r')
				offset++;
			if (offset > data + size)
				return;
		}
		offset += len;

		start = offset;
		while ((offset < (data + size)) && strncmp(boundary, offset, strlen(boundary))) {
			offset += linelen(offset, (data + size) - offset, FALSE);
			if (*offset == '\r')
				offset++;
		}
		len = offset - start;

		len = decode64(start, start, len);
		if (cistrstr(type, "x-be_attribute")) {
			index = 0;
			while (index < len) {
				name = &start[index];
				index += strlen(name) + 1;
				memcpy(&code, &start[index], sizeof(type_code));
				index += sizeof(type_code);
				memcpy(&length, &start[index], sizeof(length));
				index += sizeof(length);
				file->WriteAttr(name, code, 0, &start[index], length);
				index += length;
			}
		}
		else {
			file->Write(start, len);
			info = new BNodeInfo(file);
			type += strlen(CONTENT_TYPE);
			start = type;
			while ((*start) && (*start != ';')) {
				start++;
			}
			*start = 0;
			info->SetType(type);
			delete info;
		}
	}
}

//--------------------------------------------------------------------

void HMailView::StopLoad(void)
{
	int32		result;
	thread_id	thread;
	thread_info	info;

	if ((thread = fThread) && (get_thread_info(fThread, &info) == B_NO_ERROR)) {
		acquire_sem(fStopSem);
		wait_for_thread(thread, &result);
		fThread = 0;
		release_sem(fStopSem);
	}
}

//--------------------------------------------------------------------

bool
HMailView::get_semaphore(BWindow *window, sem_id *sem)
{
	if (!window->Lock())
		return FALSE;
	if (acquire_sem_etc(*sem, 1, B_TIMEOUT, 0) != B_NO_ERROR) {
		window->Unlock();
		return FALSE;
	}
	return TRUE;
}

//--------------------------------------------------------------------

bool
HMailView::insert(reader *info, char *line, int32 count, bool hyper)
{
	uint32			mode;
	BFont			font;
	rgb_color		c;
	rgb_color		hyper_color = {0, 0, 255, 0};
	rgb_color		normal_color = {0, 0, 0, 0};
	text_run_array	style;

	info->view->GetFontAndColor(&font, &mode, &c);
	style.count = 1;
	style.runs[0].offset = 0;
	style.runs[0].font = font;
	if (hyper)
		style.runs[0].color = hyper_color;
	else
		style.runs[0].color = normal_color;

	if (count) {
		if (!get_semaphore(info->view->Window(), info->stop_sem))
			return FALSE;
		info->view->Insert(line, count, &style);
		info->view->Window()->Unlock();
		release_sem(*info->stop_sem);
	}
	return TRUE;
}

//--------------------------------------------------------------------

bool
HMailView::parse_header(char *base, char *data, off_t size, char *boundary,
				  reader *info, off_t *processed)
{
	bool			is_bfile;
	bool			is_text;
	bool			result;
	char			*charset;
	char			*encoding;
	char			*hyper;
	char			*new_boundary;
	char			*offset;
	char			*start;
	char			*str;
	char			*type;
	char			*utf8;
	int32			index;
	int32			len;
	int32			saved_len;
	off_t			amount;
	hyper_text		*enclosure;
	Encoding 		encode;

	offset = data;
	while (1) {
		is_bfile = false;
		is_text = true;
		charset = NULL;
		encoding = NULL;
		new_boundary = NULL;
		type = NULL;

		if (boundary) {
			if ((!(offset = find_boundary(offset, boundary, (data + size) - offset))) ||
				(offset[strlen(boundary) + 1] == '-')) {
				if (processed)
					*processed = offset - data;
				return TRUE;
			}
		}

		while ((len = linelen(offset, (data + size) - offset, TRUE)) > 2) {
			if (!cistrncmp(offset, CONTENT_TYPE, strlen(CONTENT_TYPE))) {
				offset[len - 2] = 0;
				type = offset;
				if (cistrstr(offset, MIME_TEXT)
					&& !cistrstr(offset,MIME_HTML) ) {

				}else{
					is_text = FALSE;
					if (cistrstr(offset, MIME_MULTIPART)) {
						if (cistrstr(offset, "x-bfile")) {
							is_bfile = TRUE;
							start = offset + len;
							while ((start < (data + size)) && strncmp(boundary, start, strlen(boundary))) {
								index = linelen(start, (data + size) - start, TRUE);
								start[index - 2] = 0;
								if ((!cistrncmp(start, CONTENT_TYPE, strlen(CONTENT_TYPE))) &&
									(!cistrstr(start, "x-be_attribute"))) {
									type = start;
									break;
								}
								else
									start[index - 2] = '\r';
								start += index;
								if (*start == '\r')
									start++;
								if (start > data + size)
									break;
							}
						}
						else if (get_parameter(offset, "boundary=", &offset[2])) {
							offset[0] = '-';
							offset[1] = '-';
							new_boundary = offset;
						}
					}
				}
			}
			else if (!cistrncmp(offset, CONTENT_ENCODING, strlen(CONTENT_ENCODING))) {
				offset[len - 2] = 0;
				encoding = offset + strlen(CONTENT_ENCODING);
			}
			offset += len;
			if (offset >= data + size)
				return TRUE;
			if (*offset == '\r')
				offset++;
		}
		offset += len;

		if (new_boundary) {
			if (!parse_header(base, offset, (data + size) - offset, new_boundary, info, &amount))
				return FALSE;
			offset += amount;
		}
		else {
			if (boundary) {
				start = offset;
				while ((offset < (data + size)) && strncmp(boundary, offset, strlen(boundary))) {
					offset += linelen(offset, (data + size) - offset, FALSE);
					if (*offset == '\r')
						offset++;
				}
				len = offset - start;
				offset = start;
			}else
				len = (data + size) - offset;
			// Convert to utf8
			if (((is_text) && (!type)) || ((is_text) && (type) && (!cistrstr(type, "name="))))
			{
				utf8 = NULL;
				saved_len = 0;

				// Decode base64
				if ((encoding) && (cistrstr(encoding, "base64")))
				{
					saved_len = len;
					len = ::decode64(offset, offset, len);
				}else if ((encoding) && (cistrstr(encoding, "quoted-printable")))
				{// Decode quoted-printable
					saved_len = len;
					len = encode.decode_quoted_printable(offset, offset, len, true);
				}


				if ((type) && (get_parameter(type, "charset=", type))) {
					charset = type;
					saved_len = len;
					PRINT(("Charset:%s\n",charset));
					utf8 = new char[len+1];
					::strncpy(utf8,offset,len);
					utf8[len] = '\0';
					encode.ConvertToUTF8(&utf8,charset);
					len = ::strlen(utf8);
				}else{
					saved_len = len;
					utf8 = new char[len+1];
					::strncpy(utf8,offset,len);
					utf8[len] = '\0';
					encode.ConvertToUTF8(&utf8,encode.DefaultEncoding());
					len = ::strlen(utf8);
				}
				if (utf8) {
					result = strip_it(utf8, len, info);
					delete[] utf8;
					utf8 = NULL;
					if (!result)
						return FALSE;
				}
				else if (!strip_it(offset, len, info))
					return FALSE;
				if (saved_len)
					len = saved_len;
				is_text = FALSE;
			}
			else if (info->incoming) {
				if (type) {
					enclosure = new hyper_text;
					memset(enclosure, 0, sizeof(hyper_text));
					if (is_bfile)
						enclosure->type = TYPE_BE_ENCLOSURE;
					else
						enclosure->type = TYPE_ENCLOSURE;
					enclosure->content_type = new char[strlen(type) + 1];
					if (encoding) {
						enclosure->encoding = new char[strlen(encoding) + 1];
						strcpy(enclosure->encoding, encoding);
					}
					PRINT(("Type:%s\n",type));
					str = new char[strlen(type)];
					hyper = new char[strlen(str) + 256];
					if (get_parameter(type, "name=", str)) {
						enclosure->name = new char[strlen(str) + 1];
						strcpy(enclosure->name, str);
					}
					else
						sprintf(str, _("Untitled"));
					index = 0;

					BString decodedName(str);
					Encoding().Mime2UTF8(decodedName);
					while ((type[index]) && (type[index] != ';')) {
						index++;
					}
					type[index] = 0;
					sprintf(hyper, "\n<Attachment: %s (MIME type: %s)>\n",
								decodedName.String(), &type[strlen(CONTENT_TYPE)]);
					strcpy(enclosure->content_type, &type[strlen(CONTENT_TYPE)]);
					info->view->GetSelection(&enclosure->text_start,
											 &enclosure->text_end);
					enclosure->text_start++;
					enclosure->text_end += strlen(hyper) - 1;
					enclosure->file_offset = offset - base;
					enclosure->file_length = len;
					insert(info, hyper, strlen(hyper), true);
					delete[] hyper;
					delete[] str;
					info->enclosures->AddItem(enclosure);
				}
			}
			offset += len;
		}
		if (offset >= data + size)
			break;
	}
	if (processed)
		*processed = size;
	return TRUE;
}

//--------------------------------------------------------------------

static int Index_Hex[128] =
{
   -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
     -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
     -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};
#define HEX(c) (Index_Hex[(unsigned char)(c) & 0x7F])

bool
HMailView::strip_it(char* data, int32 data_len, reader *info)
{
	bool			bracket = FALSE;
	char			line[522];
	int32			count = 0;
	int32			index;
	int32			loop;
	int32			type;
	hyper_text		*enclosure;
	Encoding		encode;

	for (loop = 0; loop < data_len; loop++) {
		if ((info->quote) && ((!loop) || ((loop) && (data[loop - 1] == '\n')))) {
			strcpy(&line[count], QUOTE);
			count += strlen(QUOTE);
		}
		if ((!info->raw) && (loop) && (data[loop - 1] == '\n') && (data[loop] == '.'))
			continue;
		if ((!info->raw) && (info->incoming) && (loop < data_len - 7)) {
			type = 0;
			if (!cistrncmp(&data[loop], "http://", strlen("http://")))
				type = TYPE_HTML;
			else if (!cistrncmp(&data[loop], "https://", strlen("https://")))
				type = TYPE_HTML;
			else if (!cistrncmp(&data[loop], "ftp://", strlen("ftp://")))
				type = TYPE_FTP;
			else if (!cistrncmp(&data[loop], "mailto:", strlen("mailto:")))
				type = TYPE_MAILTO;
			if (type) {
				index = 0;
				while ((data[loop + index] != ' ') &&
					   (data[loop + index] != '\t') &&
					   (data[loop + index] != '>') &&
					   (data[loop + index] != ')') &&
					   (data[loop + index] != '"') &&
					   (data[loop + index] != '\'') &&
					   (data[loop + index] != '\r') &&
					   (!(data[loop + index] >> 7)) ) {
					index++;
				}

				if ((loop) && (data[loop - 1] == '<') && (data[loop + index] == '>')) {
					if (!insert(info, line, count - 1, FALSE))
						return FALSE;
					bracket = TRUE;
				}
				else if (!insert(info, line, count, FALSE))
					return FALSE;
				count = 0;
				enclosure = new hyper_text;
				memset(enclosure, 0, sizeof(hyper_text));
				info->view->GetSelection(&enclosure->text_start,
										 &enclosure->text_end);
				enclosure->type = type;
				enclosure->name = new char[index + 1];
				memcpy(enclosure->name, &data[loop], index);
				enclosure->name[index] = 0;
				if (bracket) {
					insert(info, &data[loop - 1], index + 2, TRUE);
					enclosure->text_end += index + 2;
					bracket = FALSE;
					loop += index;
				}
				else {
					insert(info, &data[loop], index, TRUE);
					enclosure->text_end += index;
					loop += index - 1;
				}
				info->enclosures->AddItem(enclosure);
				continue;
			}
		}
		if ((!info->raw) && (info->mime) && (data[loop] == '=')) {
			if ((loop) && (loop < data_len - 1) && (data[loop + 1] == '\r'))
				loop += 2;
			else if ((loop < data_len - 2) && (isxdigit(data[loop + 1])) &&
										 (isxdigit(data[loop + 2]))) {
				char ch = (HEX(data[loop+1]) << 4) | HEX(data[loop+2]);

				data[loop] = data[loop + 1];
				data[loop + 1] = data[loop + 2];
				data[loop + 2] = 'x';
				char *tmpBuf = new char[2];
				tmpBuf[0] = ch;
				tmpBuf[1] = '\0';
				encode.ConvertToUTF8(&tmpBuf,B_ISO1_CONVERSION);

				int32 charLen = strlen(tmpBuf);
				for(int32 k = 0;k < charLen;k++)
					line[count++] = tmpBuf[k];//strtol(&data[loop], NULL, 16);

				loop += 2;
				delete[] tmpBuf;
			}
			else
				line[count++] = data[loop];
		}
		else if (data[loop] != '\r')
			line[count++] = data[loop];

		if ((count > 511) || ((count) && (loop == data_len - 1))) {
			if (!insert(info, line, count, FALSE))
				return FALSE;
			count = 0;
		}
	}
	return TRUE;
}

//====================================================================
// case-insensitive version of strncmp
//

int32
HMailView::cistrncmp(char *str1, char *str2, int32 max)
{
	char		c1;
	char		c2;
	int32		loop;

	for (loop = 0; loop < max; loop++) {
		c1 = *str1++;
		if ((c1 >= 'A') && (c1 <= 'Z'))
			c1 += ('a' - 'A');
		c2 = *str2++;
		if ((c2 >= 'A') && (c2 <= 'Z'))
			c2 += ('a' - 'A');
		if (c1 == c2) {
		}
		else if (c1 < c2)
			return -1;
		else if ((c1 > c2) || (!c2))
			return 1;
	}
	return 0;
}


//--------------------------------------------------------------------
// case-insensitive version of strstr
//

char*
HMailView::cistrstr(char *cs, char *ct)
{
	char		c1;
	char		c2;
	int32		cs_len;
	int32		ct_len;
	int32		loop1;
	int32		loop2;

	cs_len = strlen(cs);
	ct_len = strlen(ct);
	for (loop1 = 0; loop1 < cs_len; loop1++) {
		if (cs_len - loop1 < ct_len)
			goto done;
		for (loop2 = 0; loop2 < ct_len; loop2++) {
			c1 = cs[loop1 + loop2];
			if ((c1 >= 'A') && (c1 <= 'Z'))
				c1 += ('a' - 'A');
			c2 = ct[loop2];
			if ((c2 >= 'A') && (c2 <= 'Z'))
				c2 += ('a' - 'A');
			if (c1 != c2)
				goto next;
		}
		return(&cs[loop1]);
next:;
	}
done:;
	return(NULL);
}


//--------------------------------------------------------------------
// Un-fold field and add items to dst
//

void
HMailView::extract(char **dst, char *src)
{
	bool		remove_ws = TRUE;
	int32		comma = 0;
	int32		count = 0;
	int32		index = 0;
	int32		len;

	if (strlen(*dst))
		comma = 2;

	for (;;) {
		if (src[index] == '\r') {
			if (count) {
				len = strlen(*dst);
				*dst = (char *)realloc(*dst, len + count + comma + 1);
				if (comma) {
					(*dst)[len++] = ',';
					(*dst)[len++] = ' ';
					comma = 0;
				}
				memcpy(&((*dst)[len]), &src[index - count], count);
				(*dst)[len + count] = 0;
				count = 0;

				if (src[index + 1] == '\n')
					index++;
				if ((src[index + 1] != ' ') && (src[index + 1] != '\t'))
					break;
			}
		}
		else {
			if ((remove_ws) && ((src[index] == ' ') || (src[index] == '\t'))) {
			}
			else {
				remove_ws = FALSE;
				count++;
			}
		}
		index++;
	}
}


//--------------------------------------------------------------------
// return length of \n terminated line
//

int32
HMailView::linelen(char *str, int32 len, bool header)
{
	int32		loop;

	for (loop = 0; loop < len; loop++) {
		if (str[loop] == '\n') {
			if (
			     ((loop + 1) == len) ||
			     (!header)           ||
			     (loop < 2)          ||
			     (
			       (header)               &&
			       (str[loop + 1] != ' ') &&
			       (str[loop + 1] != '\t')
			     )
			   ) {
				return loop + 1;
			}
		}
	}
	return len;
}

//--------------------------------------------------------------------
// get named parameter from string
//

bool
HMailView::get_parameter(char *src, char *param, char *dst)
{
	char		*offset;
	int32		len;

	if ((offset = cistrstr(src, param))) {
		offset += strlen(param);
		len = strlen(src) - (offset - src);
		if (*offset == '"') {
			offset++;
			len = 0;
			while (offset[len] != '"') {
				len++;
			}
		}
		strcpy(dst, offset);
		dst[len] = '\0';
		char *p = strchr(dst,';');
		if(p)
			p[0] = '\0';
		return TRUE;
	}
	return FALSE;

/*	char		*offset;
	int32		len;

	if ((offset = cistrstr(src, param))) {
		offset += strlen(param);
		len = strlen(src) - (offset - src);
		if (*offset == '"') {
			offset++;
			len = 0;
			while (offset[len] != '"') {
				len++;
			}
			offset[len] = 0;
		}
		strcpy(dst, offset);
		return TRUE;
	}
	return FALSE;
*/
}


//--------------------------------------------------------------------
// search buffer for boundary
//

char*
HMailView::find_boundary(char *buf, char *boundary, int32 len)
{
	char	*offset;

	offset = buf;
	while (strncmp(boundary, offset, strlen(boundary))) {
		offset += linelen(offset, (buf + len) - offset + 1, FALSE);
		if (*offset == '\r')
			offset++;
		if (offset >= buf + len)
			return NULL;
	}
	return offset;
}

/***********************************************************
 * Find
 ***********************************************************/
void
HMailView::Find(const char* inText)
{
	MakeFocus(true);
	int32 len = TextLength();

	int32 start,end;
	GetSelection(&start,&end);

	const char* text = Text();
	int32 targetLen = strlen(inText);
	for(int32 i = end;i < len;i++)
	{
		// Find
		if(::strncasecmp(&text[i],inText,targetLen) == 0)
		{
			PRINT(("hit:%d %d\n",i,i+targetLen));
			Select(i,i+targetLen);
			break;
		}
		// Not found
		if(i == len-1)
		{
			beep();
			Select(0,0);
			break;
		}
	}
}

/***********************************************************
 * GetDragParameter
 ***********************************************************/
void
HMailView::GetDragParameters(BMessage *drag
						,BBitmap **bitmap
						,BPoint *point
						,BHandler **handler)
{

	int32 start,end;
	GetSelection(&start,&end);

	hyper_text		*enclosure;

	int32 items = fEnclosures->CountItems();
	for (int32 loop = 0; loop < items; loop++) {
		enclosure = (hyper_text *)fEnclosures->ItemAt(loop);
		if ((start >= enclosure->text_start) && (start < enclosure->text_end)) {
			PRINT(("enclosure\n"));
			drag->MakeEmpty();

			BString decodedName(enclosure->name);
			Encoding().Mime2UTF8(decodedName);

			off_t size = enclosure->file_length;
			fFile->Seek(enclosure->file_offset,SEEK_SET);
			char *data = new char[size+1];
			fFile->Read(data,size);
			data[size] = '\0';
			size= ::decode64(data, data, size);
			drag->what = B_SIMPLE_DATA;
			drag->AddInt32("be:actions",B_COPY_TARGET);
			drag->AddString("be:types",B_FILE_MIME_TYPE);
			drag->AddString("be:filetypes",enclosure->content_type);
			drag->AddString("be:clip_name",decodedName.String());
			BMessage dataMsg(B_SIMPLE_DATA);
			dataMsg.AddData("data",B_ANY_TYPE,data,size);
			drag->AddMessage("be:originator-data",&dataMsg);
			drag->PrintToStream();

			delete[] data;
			return;
		}
	}
	_inherited::GetDragParameters(drag,bitmap,point,handler);
}

//====================================================================

TSavePanel::TSavePanel(hyper_text *enclosure, HMailView *view)
		   :BFilePanel(B_SAVE_PANEL)
{
	fEnclosure = enclosure;
	fView = view;
	if (enclosure->name)
		SetSaveFileName(enclosure->name);
}

void
TSavePanel::SetSaveFileName(const char* name)
{
	BString decodedName(name);
	Encoding().Mime2UTF8(decodedName);
	SetSaveText(decodedName.String());
}

//--------------------------------------------------------------------

void TSavePanel::SendMessage(const BMessenger */*messenger*/, BMessage *msg)
{
	const char	*name = NULL;
	BMessage	save(M_SAVE);
	entry_ref	ref;

	if ((!msg->FindRef("directory", &ref)) && (!msg->FindString("name", &name))) {
		save.AddPointer("enclosure", fEnclosure);
		save.AddString("name", name);
		save.AddRef("directory", &ref);
		fView->Window()->PostMessage(&save, fView);
	}
}

//--------------------------------------------------------------------

void TSavePanel::SetEnclosure(hyper_text *enclosure)
{
	fEnclosure = enclosure;
	if (enclosure->name)
		SetSaveFileName(enclosure->name);
	else
		SetSaveText("");
	if (!IsShowing())
		Show();
	Window()->Activate();
}

