/*****************************************************************************
 * PlayListWindow.cpp: beos interface
 *****************************************************************************
 * Copyright (C) 1999, 2000, 2001 VideoLAN
 * $Id: PlayListWindow.cpp,v 1.7 2003/02/01 12:01:11 stippi Exp $
 *
 * Authors: Jean-Marc Dressler <polux@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Tony Castley <tony@castley.net>
 *          Richard Shepherd <richard@rshepherd.demon.co.uk>
 *          Stephan Aßmus <stippi@yellowbites.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/* System headers */
#include <InterfaceKit.h>
#include <StorageKit.h>
#include <string.h>

/* VLC headers */
#include <vlc/vlc.h>
#include <vlc/intf.h>

/* BeOS interface headers */
#include "VlcWrapper.h"
#include "InterfaceWindow.h"
#include "ListViews.h"
#include "MsgVals.h"
#include "PlayListWindow.h"

enum
{
	MSG_SELECT_ALL			= 'sall',
	MSG_SELECT_NONE			= 'none',
	MSG_RANDOMIZE			= 'rndm',
	MSG_SORT_NAME			= 'srtn',
	MSG_SORT_PATH			= 'srtp',
	MSG_REMOVE				= 'rmov',
	MSG_REMOVE_ALL			= 'rmal',

	MSG_SELECTION_CHANGED	= 'slch',
};


/*****************************************************************************
 * PlayListWindow::PlayListWindow
 *****************************************************************************/
PlayListWindow::PlayListWindow( BRect frame, const char* name,
								InterfaceWindow* mainWindow,
								intf_thread_t *p_interface )
	:	BWindow( frame, name, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
				 B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS ),
		fMainWindow( mainWindow )
{
	p_intf = p_interface;
    p_wrapper = p_intf->p_sys->p_wrapper;
    
    SetName( "playlist" );

    // set up the main menu bar
	fMenuBar = new BMenuBar( BRect(0.0, 0.0, frame.Width(), 15.0), "main menu",
							 B_FOLLOW_NONE, B_ITEMS_IN_ROW, false );

    AddChild( fMenuBar );

	// Add the File menu
	BMenu *fileMenu = new BMenu( "File" );
	fMenuBar->AddItem( fileMenu );
	BMenuItem* item = new BMenuItem( "Open File" B_UTF8_ELLIPSIS,
									 new BMessage( OPEN_FILE ), 'O' );
	item->SetTarget( fMainWindow );
	fileMenu->AddItem( item );

	CDMenu* cd_menu = new CDMenu( "Open Disc" );
	fileMenu->AddItem( cd_menu );

	fileMenu->AddSeparatorItem();
	item = new BMenuItem( "Close",
						  new BMessage( B_QUIT_REQUESTED ), 'W' );
	fileMenu->AddItem( item );

	// Add the Edit menu
	BMenu *editMenu = new BMenu( "Edit" );
	fMenuBar->AddItem( editMenu );
	fSelectAllMI = new BMenuItem( "Select All",
								  new BMessage( MSG_SELECT_ALL ), 'A' );
	editMenu->AddItem( fSelectAllMI );
	fSelectNoneMI = new BMenuItem( "Select None",
								   new BMessage( MSG_SELECT_NONE ), 'A', B_SHIFT_KEY );
	editMenu->AddItem( fSelectNoneMI );

	editMenu->AddSeparatorItem();
	fSortNameMI = new BMenuItem( "Sort by Name",
								 new BMessage( MSG_SORT_NAME ), 'N' );
fSortNameMI->SetEnabled( false );
	editMenu->AddItem( fSortNameMI );
	fSortPathMI = new BMenuItem( "Sort by Path",
								 new BMessage( MSG_SORT_PATH ), 'P' );
fSortPathMI->SetEnabled( false );
	editMenu->AddItem( fSortPathMI );
	fRandomizeMI = new BMenuItem( "Randomize",
								  new BMessage( MSG_RANDOMIZE ), 'R' );
fRandomizeMI->SetEnabled( false );
	editMenu->AddItem( fRandomizeMI );
	editMenu->AddSeparatorItem();
	fRemoveMI = new BMenuItem( "Remove",
						  new BMessage( MSG_REMOVE ) );
	editMenu->AddItem( fRemoveMI );
	fRemoveAllMI = new BMenuItem( "Remove All",
								  new BMessage( MSG_REMOVE_ALL ) );
	editMenu->AddItem( fRemoveAllMI );

	// make menu bar resize to correct height
	float menuWidth, menuHeight;
	fMenuBar->GetPreferredSize( &menuWidth, &menuHeight );
	// don't change next line! it's a workarround!
	fMenuBar->ResizeTo( frame.Width(), menuHeight );

	frame = Bounds();
	frame.top += fMenuBar->Bounds().IntegerHeight() + 1;
	frame.right -= B_V_SCROLL_BAR_WIDTH;

	fListView = new PlaylistView( frame, fMainWindow, p_wrapper,
								  new BMessage( MSG_SELECTION_CHANGED ) );
	fBackgroundView = new BScrollView( "playlist scrollview",
									   fListView, B_FOLLOW_ALL_SIDES,
									   0, false, true,
									   B_NO_BORDER );

	AddChild( fBackgroundView );

	// be up to date
	UpdatePlaylist();
	FrameResized( Bounds().Width(), Bounds().Height() );
	SetSizeLimits( menuWidth * 2.0, menuWidth * 6.0,
				   menuHeight * 5.0, menuHeight * 25.0 );

	UpdatePlaylist( true );
	// start window thread in hidden state
	Hide();
	Show();
}

/*****************************************************************************
 * PlayListWindow::~PlayListWindow
 *****************************************************************************/
PlayListWindow::~PlayListWindow()
{
}

/*****************************************************************************
 * PlayListWindow::QuitRequested
 *****************************************************************************/
bool
PlayListWindow::QuitRequested()
{
	Hide(); 
	return false;
}

/*****************************************************************************
 * PlayListWindow::MessageReceived
 *****************************************************************************/
void
PlayListWindow::MessageReceived( BMessage * p_message )
{
	switch ( p_message->what )
	{
		case OPEN_DVD:
		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			// forward to interface window
			fMainWindow->PostMessage( p_message );
			break;
		case MSG_SELECT_ALL:
			fListView->Select( 0, fListView->CountItems() - 1 );
			break;
		case MSG_SELECT_NONE:
			fListView->DeselectAll();
			break;
		case MSG_RANDOMIZE:
			break;
		case MSG_SORT_NAME:
			break;
		case MSG_SORT_PATH:
			break;
		case MSG_REMOVE:
			fListView->RemoveSelected();
			break;
		case MSG_REMOVE_ALL:
			fListView->Select( 0, fListView->CountItems() - 1 );
			fListView->RemoveSelected();
			break;
		case MSG_SELECTION_CHANGED:
			_CheckItemsEnableState();
			break;
		case B_MODIFIERS_CHANGED:
			fListView->ModifiersChanged();
			break;
		default:
			BWindow::MessageReceived( p_message );
			break;
	}
}

/*****************************************************************************
 * PlayListWindow::FrameResized
 *****************************************************************************/
void
PlayListWindow::FrameResized(float width, float height)
{
	BRect r(Bounds());
	fMenuBar->MoveTo(r.LeftTop());
	fMenuBar->ResizeTo(r.Width(), fMenuBar->Bounds().Height());
	r.top += fMenuBar->Bounds().Height() + 1.0;
	fBackgroundView->MoveTo(r.LeftTop());
	// the "+ 1.0" is to make the scrollbar
	// be partly covered by the window border
	fBackgroundView->ResizeTo(r.Width() + 1.0, r.Height() + 1.0);
}

/*****************************************************************************
 * PlayListWindow::ReallyQuit
 *****************************************************************************/
void
PlayListWindow::ReallyQuit()
{
    Lock();
    Hide();
    Quit();
}

/*****************************************************************************
 * PlayListWindow::UpdatePlaylist
 *****************************************************************************/
void
PlayListWindow::UpdatePlaylist( bool rebuild )
{
	if ( rebuild )
		fListView->RebuildList();
	fListView->SetCurrent( p_wrapper->PlaylistCurrent() );
	fListView->SetPlaying( p_wrapper->IsPlaying() );

	_CheckItemsEnableState();
}

/*****************************************************************************
 * PlayListWindow::_CheckItemsEnableState
 *****************************************************************************/
void
PlayListWindow::_CheckItemsEnableState() const
{
	// check if one item selected
	int32 test = fListView->CurrentSelection( 0 );
	bool enable1 = test >= 0;
	// check if at least two items selected
//	test = fListView->CurrentSelection( 1 );
//	bool enable2 = test >= 0;
	bool notEmpty = fListView->CountItems() > 0;
	_SetMenuItemEnabled( fSelectAllMI, notEmpty );
	_SetMenuItemEnabled( fSelectNoneMI, enable1 );
//	_SetMenuItemEnabled( fSortNameMI, enable2 );
//	_SetMenuItemEnabled( fSortPathMI, enable2 );
//	_SetMenuItemEnabled( fRandomizeMI, enable2 );
	_SetMenuItemEnabled( fRemoveMI, enable1 );
	_SetMenuItemEnabled( fRemoveAllMI, notEmpty );
}

/*****************************************************************************
 * PlayListWindow::_SetMenuItemEnabled
 *****************************************************************************/
void
PlayListWindow::_SetMenuItemEnabled( BMenuItem* item, bool enabled ) const
{
	// this check should actally be done in BMenuItem::SetEnabled(), but it is not...
	if ( item->IsEnabled() != enabled )
		item->SetEnabled( enabled );
}
