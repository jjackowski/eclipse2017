/*
 * This file is part of the Eclipse2017 project. It is subject to the GPLv3
 * license terms in the LICENSE file found in the top-level directory of this
 * distribution and at
 * https://github.com/jjackowski/eclipse2017/blob/master/LICENSE.
 * No part of the Eclipse2017 project, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 *
 * Copyright (C) 2017  Jeff Jackowski
 */
#ifndef PAGE_HPP
#define PAGE_HPP

#include <duds/hardware/devices/displays/TextDisplayStream.hpp>
#include <boost/noncopyable.hpp>
#include "DisplayStuff.hpp"

/**
 * An interface to use for displayable pages.
 */
class Page : boost::noncopyable {
public:
	virtual ~Page() { };
	/**
	 * Ways a page may be selected for display.
	 */
	enum SelectionCause {
		/**
		 * Page selection by automatic cycling.
		 */
		SelectAuto,
		/**
		 * Page selection by user input. Some pages that may be skipped over
		 * by automatic cycling can still be selected by the user.
		 */
		SelectUser
	};
	/**
	 * Responses to a page selection.
	 */
	enum SelectionResponse {
		/**
		 * The page accepts the selection.
		 */
		SelectPage,
		/**
		 * The page will be skipped over.
		 */
		SkipPage
	};
	/**
	 * Handles a request to select the page for display. The page will not be
	 * shown if SkipPage is returned.
	 */
	virtual SelectionResponse select(
		const DisplayInfo &di,
		SelectionCause cause
	) = 0;
	/**
	 * Writes the page to the display when the page was not previously visible.
	 * The rightmost 9 characters of the first row must not be used.
	 * @pre  The display's cursor will be at the start of the second row.
	 */
	virtual void show(
		const DisplayInfo &di,
		duds::hardware::devices::displays::TextDisplayStream &tds
	) = 0;
	/**
	 * Updates the page already on the display; called normally once per second.
	 * The rightmost 9 characters of the first row must not be used.
	 * @pre  The display's cursor will be at the start of the second row.
	 */
	virtual void update(
		const DisplayInfo &di,
		duds::hardware::devices::displays::TextDisplayStream &tds
	) = 0;
};

#endif        //  #ifndef PAGE_HPP
