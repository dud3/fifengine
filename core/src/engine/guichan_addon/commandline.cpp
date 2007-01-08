/***************************************************************************
 *   Copyright (C) 2005-2007 by the FIFE Team                              *
 *   fife-public@lists.sourceforge.net                                     *
 *   This file is part of FIFE.                                            *
 *                                                                         *
 *   FIFE is free software; you can redistribute it and/or modify          *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA              *
 ***************************************************************************/

// Standard C++ library includes
#include <cassert>

// 3rd party library includes
#include <boost/bind.hpp>

// FIFE includes
// These includes are split up in two parts, separated by one empty line
// First block: files included from the FIFE root src directory
// Second block: files included from the same folder
#include "input/manager.h"
#include "video/gui/guimanager.h"
#include "log.h"
#include "timeevent.h"
#include "timemanager.h"

#include "commandline.h"

namespace FIFE {
	using namespace gcn;


	CommandLine::CommandLine() : gcn::TextField("") {
		m_history_position = 0;

		m_blinkTimer.setInterval(500);
		m_blinkTimer.setCallback(boost::bind(&CommandLine::toggleCaretVisible,this));
		m_blinkTimer.start();

		m_suppressBlinkTimer.setInterval(2000);
		m_suppressBlinkTimer
			.setCallback(boost::bind(&CommandLine::startBlinking,this));
	}

	CommandLine::~CommandLine() {
	}

	void CommandLine::toggleCaretVisible() {
		m_caretVisible = !m_caretVisible;
	}

	void CommandLine::stopBlinking() {
		m_suppressBlinkTimer.start();
		m_blinkTimer.stop();
		m_caretVisible = true;
	}

	void CommandLine::startBlinking() {
		m_suppressBlinkTimer.stop();
		m_blinkTimer.start();
	}

	// directly copied from gcn::textfield with one change
	void CommandLine::keyPress(const gcn::Key & key) {
		//Log("DEBUG-key") << key.getValue();
		if (key.getValue() == Key::LEFT && mCaretPosition > 0)
		{
			--mCaretPosition;
		}
		else if (key.getValue() == Key::RIGHT && mCaretPosition < mText.size())
		{
			++mCaretPosition;
		}
		else if (key.getValue() == Key::DOWN && !m_history.empty())
		{
			if( m_history_position < m_history.size() ) {
				++m_history_position;
				if( m_history_position == m_history.size() ) {
					setText( m_cmdline ); 
				} else {
					setText( m_history[m_history_position] ); 
				}
			};
		}
		else if (key.getValue() == Key::UP && !m_history.empty())
		{
			if( m_history_position > 0 ) {
				if( m_history_position == m_history.size() ) {
					m_cmdline = mText; 
				}
				--m_history_position;
				setText( m_history[m_history_position] ); 
			};
		}
		else if (key.getValue() == Key::DELETE && mCaretPosition < mText.size())
		{
			mText.erase(mCaretPosition, 1);
		}
		else if (key.getValue() == Key::BACKSPACE && mCaretPosition > 0)
		{
			mText.erase(mCaretPosition - 1, 1);
			--mCaretPosition;
		}
		else if (key.getValue() == Key::ENTER)
		{
			if( mText != "" ) {
				if(m_callback) {
					m_callback( mText );
				}
				m_history.push_back( mText ); 
				m_history_position = m_history.size();
				setText("");
			}
		}
		else if (key.getValue() == Key::HOME)
		{
			mCaretPosition = 0;
		}    
		else if (key.getValue() == Key::END)
		{
			mCaretPosition = mText.size();
		}    
		else if (key.isCharacter())
		{
			m_cmdline = mText;
			m_history_position = m_history.size();

			mText.insert(mCaretPosition, std::string(1,char(key.getValue())));
			++mCaretPosition;
		}
		stopBlinking();
		fixScroll();
	}

	void CommandLine::drawCaret(gcn::Graphics * graphics, int x) {
		if( !m_caretVisible )
			return;

		graphics->setColor(getForegroundColor());
		graphics->drawLine(x, getHeight() - 2, x, 1);
		graphics->drawLine(x+1, getHeight() - 2, x+1, 1);
	}


	void CommandLine::setCallback(const type_callback& cb) {
		m_callback = cb;
	}

}
/* vim: set noexpandtab: set shiftwidth=2: set tabstop=2: */
